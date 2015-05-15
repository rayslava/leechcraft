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

#include "freebsdbackend.h"
#include <memory>
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/pcpu.h>
#include <sys/sysctl.h>
#include <QtDebug>

namespace LeechCraft
{
namespace CpuLoad
{
	void FreeBSDBackend::Update ()
	{
		const auto kvm = kvm_open (nullptr, nullptr, nullptr, O_RDONLY, "LeechCraft CpuLoad"); // TODO proper error reporting
		if (!kvm)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open kernel memory";
			return;
		}

		const auto cpuCount = kvm_getncpus (kvm);
		for (int i = 0; i < cpuCount; ++i)
		{
			std::unique_ptr<void, decltype (&free)> rawPcpu { kvm_getpcpu (kvm, i), &free };
			auto pcp = static_cast<pcpu*> (rawPcpu.get ());
		}

		kvm_close (kvm);
	}

	int FreeBSDBackend::GetCpuCount () const
	{
		return 0;
	}

	QMap<LoadPriority, LoadTypeInfo> FreeBSDBackend::GetLoads (int cpu) const
	{
		return {};
	}
}
}