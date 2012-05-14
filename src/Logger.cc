#include "Logger.h"

using namespace hue;

Logger::Level Logger::currentLevel = Logger::Debug;
volatile uint8_t Logger::isATTY = 0;
