/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "filetransfer.h"
#include <tox/tox.h>
#include <util/sll/futures.h>
#include <util/sll/delayedexecutor.h>
#include "toxthread.h"
#include "util.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	FileTransfer::FileTransfer (const QString& azothId,
			const QByteArray& pubkey,
			const QString& filename,
			const std::shared_ptr<ToxThread>& thread,
			QObject *parent)
	: QObject { parent }
	, AzothId_ { azothId }
	, PubKey_ { pubkey }
	, FilePath_ { filename }
	, Thread_ { thread }
	, File_ { filename }
	, Filesize_ { File_.size () }
	{
		if (!File_.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open local file"
					<< filename;

			new Util::DelayedExecutor
			{
				[this]
				{
					emit errorAppeared (TEFileAccessError,
							tr ("Error opening local file: %1.")
								.arg (File_.errorString ()));
					emit stateChanged (TransferState::TSFinished);
				}
			};

			return;
		}

		connect (Thread_.get (),
				SIGNAL (gotFileControl (qint32, qint8, qint8, QByteArray)),
				this,
				SLOT (handleFileControl (qint32, qint8, qint8, QByteArray)));

		const auto sendScheduler = [this]
		{
			return Thread_->ScheduleFunction ([this] (Tox *tox)
					{
						const auto& name = FilePath_.section ('/', -1, -1).toUtf8 ();
						FriendNum_ = GetFriendId (tox, PubKey_);
						return tox_new_file_sender (tox,
								FriendNum_,
								static_cast<uint64_t> (Filesize_),
								reinterpret_cast<const uint8_t*> (name.constData ()),
								name.size ());
					});
		};
		Util::ExecuteFuture (sendScheduler,
				[this] (int filenum)
				{
					if (filenum >= 0)
						return;
					qWarning () << Q_FUNC_INFO
							<< "unable to send file";
				},
				this);

		new Util::DelayedExecutor
		{
			[this] { emit stateChanged (TransferState::TSOffer); }
		};
	}

	QString FileTransfer::GetSourceID () const
	{
		return AzothId_;
	}

	QString FileTransfer::GetName () const
	{
		return FilePath_;
	}

	qint64 FileTransfer::GetSize () const
	{
		return Filesize_;
	}

	QString FileTransfer::GetComment () const
	{
		return {};
	}

	TransferDirection FileTransfer::GetDirection () const
	{
		return TransferDirection::TDOut;
	}

	void FileTransfer::Accept (const QString&)
	{
	}

	void FileTransfer::Abort ()
	{
	}

	void FileTransfer::HandleAccept ()
	{
		State_ = State::Transferring;
		TransferChunk ();

		emit stateChanged (TSTransfer);
	}

	void FileTransfer::HandleKill ()
	{
		TransferAllowed_ = false;
		emit errorAppeared (TEAborted, tr ("Remote party aborted file transfer."));
		emit stateChanged (TSFinished);
		State_ = State::Idle;
	}

	void FileTransfer::HandlePause ()
	{
		TransferAllowed_ = false;
		State_ = State::Paused;
	}

	void FileTransfer::HandleResume ()
	{
		TransferAllowed_ = true;
		State_ = State::Transferring;
		TransferChunk ();
	}

	void FileTransfer::HandleResumeBroken (const QByteArray& data)
	{
		if (data.size () < 8)
		{
			qWarning () << Q_FUNC_INFO
					<< "insufficient data to get the 64-bit position value:"
					<< data.toHex ();
			return;
		}

		const uint64_t pos = *reinterpret_cast<const uint64_t*> (data.constData ());
		qDebug () << Q_FUNC_INFO
				<< "would resume broken transfer starting from position"
				<< pos
				<< "of"
				<< File_.size ()
				<< "; current pos:"
				<< File_.pos ();

		if (!File_.seek (pos))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to seek to"
					<< pos
					<< "of"
					<< File_.size ()
					<< "; error:"
					<< File_.errorString ();
			return;
		}

		const auto finishSheduler = [this]
		{
			return Thread_->ScheduleFunction ([this] (Tox *tox)
					{
						return tox_file_send_control (tox, FriendNum_, 0, FileNum_, TOX_FILECONTROL_ACCEPT, nullptr, 0);
					});
		};
		Util::ExecuteFuture (finishSheduler,
				[this] (int sendRes)
				{
					if (!sendRes)
					{
						emit stateChanged (TSFinished);
						TransferChunk ();
						return;
					}

					qWarning () << Q_FUNC_INFO
							<< "error finalizing the file transfer";
					emit errorAppeared (TEProtocolError, tr ("Error transferring another chunk."));
					emit stateChanged (TSFinished);
				},
				this);
	}

	void FileTransfer::TransferChunk ()
	{
		if (!TransferAllowed_)
			return;

		if (!File_.atEnd ())
		{
			const auto sendScheduler = [this]
			{
				return Thread_->ScheduleFunction ([this] (Tox *tox)
						{
							const int chunkSize = tox_file_data_size (tox, FriendNum_);
							const auto& data = File_.read (chunkSize);
							return tox_file_send_data (tox, FriendNum_, FileNum_,
									reinterpret_cast<const uint8_t*> (data.constData ()), data.size ());
						});
			};
			Util::ExecuteFuture (sendScheduler,
					[this] (int sendRes)
					{
						if (!sendRes)
						{
							emit transferProgress (File_.pos (), File_.size ());
							TransferChunk ();
							return;
						}

						qWarning () << Q_FUNC_INFO
								<< "error sending the file"
								<< sendRes;
						emit errorAppeared (TEProtocolError, tr ("Error transferring another chunk."));
						emit stateChanged (TSFinished);
					},
					this);
		}
		else
		{
			const auto finishSheduler = [this]
			{
				return Thread_->ScheduleFunction ([this] (Tox *tox)
						{
							return tox_file_send_control (tox, FriendNum_, 0, FileNum_, TOX_FILECONTROL_FINISHED, nullptr, 0);
						});
			};
			Util::ExecuteFuture (finishSheduler,
					[this] (int sendRes)
					{
						if (!sendRes)
						{
							emit stateChanged (TSFinished);
							return;
						}

						qWarning () << Q_FUNC_INFO
								<< "error finalizing the file transfer";
						emit errorAppeared (TEProtocolError, tr ("Error transferring another chunk."));
						emit stateChanged (TSFinished);
					},
					this);
		}
	}

	void FileTransfer::handleFileControl (qint32 friendNum,
			qint8 fileNum, qint8 type, const QByteArray& data)
	{
		if (friendNum != FriendNum_ || fileNum != FileNum_)
			return;

		switch (State_)
		{
		case State::Waiting:
			switch (type)
			{
			case TOX_FILECONTROL_ACCEPT:
				HandleAccept ();
				break;
			case TOX_FILECONTROL_KILL:
				HandleKill ();
				break;
			default:
				qWarning () << Q_FUNC_INFO
						<< "unexpected control type in Waiting state:"
						<< type;
				break;
			}
			break;
		case State::Transferring:
			switch (type)
			{
			case TOX_FILECONTROL_KILL:
				HandleKill ();
				break;
			case TOX_FILECONTROL_PAUSE:
				HandlePause ();
				break;
			case TOX_FILECONTROL_RESUME_BROKEN:
				HandleResumeBroken (data);
				break;
			default:
				qWarning () << Q_FUNC_INFO
						<< "unexpected control type in Transferring state:"
						<< type;
				break;
			}
			break;
		case State::Paused:
			switch (type)
			{
			case TOX_FILECONTROL_ACCEPT:
				HandleResume ();
				break;
			case TOX_FILECONTROL_KILL:
				HandleKill ();
				break;
			default:
				qWarning () << Q_FUNC_INFO
						<< "unexpected control type in Killed state:"
						<< type;
				break;
			}
			break;
		}
	}
}
}
}
