/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "anhero.h"
#include <QIcon>
#include <QApplication>
#include <QX11Info>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <util/util.h>
#include <interfaces/core/icoreproxy.h>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

namespace LeechCraft
{
namespace AnHero
{
	namespace
	{
		QByteArray AppPath_;
		QByteArray AppDir_;
		QByteArray AppVersion_;
		QByteArray AppArgs_;

		void CloseFiles ()
		{
			rlimit rlp;
			getrlimit (RLIMIT_NOFILE, &rlp);
			for (rlim_t i = 3; i < rlp.rlim_cur; ++i)
				close (i);
		}

		void Exec (const char **argv)
		{
			pid_t pid = fork ();
			switch (pid)
			{
			case -1:
				fprintf (stderr, "%s: failed to fork(), errno: %d\n", Q_FUNC_INFO, errno);
				break;
			case 0:
				CloseFiles ();
				execvp (argv[0], const_cast<char**> (argv));
				fprintf (stderr, "%s: failed to exec(), errno: %d\n", Q_FUNC_INFO, errno);
				_exit (253);
				break;
			default:
				alarm (0);
				while (waitpid (-1, nullptr, 0) != pid)
					;
				break;
			}
		}

		void DefaultCrashHandler (int signal)
		{
			static uint8_t RecGuard = 0;
			if (RecGuard++)
				return;

			alarm (5);

			char sigtxt [10];
			sprintf (sigtxt, "%d", signal);

			char pidtxt [10];
			sprintf (pidtxt, "%lld", QCoreApplication::applicationPid ());

#ifdef Q_OS_MAC
			char crashprocess [1024] = { 0 };
			sprintf (crashprocess, "%s/lc_anhero_crashprocess", AppDir_.constData ());
#endif

			const char *argv [] =
			{
#ifndef Q_OS_MAC
				"lc_anhero_crashprocess",
#else
				crashprocess,
#endif
#ifdef HAVE_X11
				"-display",
				QX11Info::display () ? XDisplayString (QX11Info::display ()) : getenv ("DISPLAY"),
#endif
				"--signal",
				sigtxt,
				"--pid",
				pidtxt,
				"--path",
				AppPath_.constData (),
				"--version",
				AppVersion_.constData (),
				"--cmdline",
				AppArgs_.constData (),
				nullptr
			};

			Exec (argv);
			_exit (255);
		}

		void SetCrashHandler (void (*handler) (int))
		{
			sigset_t mask;
			sigemptyset (&mask);

			auto add = [&mask, handler] (int sig)
			{
				signal (sig, handler);
				sigaddset (&mask, sig);
			};
#ifdef SIGSEGV
			add (SIGSEGV);
#endif
#ifdef SIGBUS
			add (SIGBUS);
#endif
#ifdef SIGFPE
			add (SIGFPE);
#endif
#ifdef SIGILL
			add (SIGILL);
#endif
#ifdef SIGABRT
			add (SIGABRT);
#endif

			sigprocmask (SIG_UNBLOCK, &mask, 0);
		}
	}

	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("anhero");

		const auto& args = QApplication::arguments ();
		if (args.contains ("-noanhero"))
			return;

#ifdef Q_OS_MAC
		if (!QFile::exists ("/usr/bin/gdb"))
			return;
#endif

		AppPath_ = QCoreApplication::applicationFilePath ().toUtf8 ();
		AppDir_ = QCoreApplication::applicationDirPath ().toUtf8 ();
		AppVersion_ = proxy->GetVersion ().toUtf8 ();
		AppArgs_ = args.join (" ").toUtf8 ();
		SetCrashHandler (DefaultCrashHandler);
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.AnHero";
	}

	QString Plugin::GetName () const
	{
		return "AnHero";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Crash handler");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/anhero.svg");
		return icon;
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_anhero, LeechCraft::AnHero::Plugin);
