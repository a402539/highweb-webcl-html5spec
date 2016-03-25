// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HistogramTester_h
#define HistogramTester_h

#include "platform/Histogram.h"
#include "wtf/OwnPtr.h"

namespace base {
class HistogramTester;
}

namespace blink {

// Blink interface for base::HistogramTester.
class HistogramTester {
public:
    HistogramTester();
    ~HistogramTester();

    void expectUniqueSample(const std::string& name, base::HistogramBase::Sample, base::HistogramBase::Count) const;

private:
    OwnPtr<base::HistogramTester> m_histogramTester;
};

} // namespace blink

#endif // HistogramTester_h