// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#include "Logger.h"

using namespace hue;

Logger::Level Logger::currentLevel = Logger::Debug;
volatile uint8_t Logger::isATTY = 0;
