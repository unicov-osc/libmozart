/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/timer>

std::chrono::time_point<std::chrono::high_resolution_clock>
mpp::timer::m_timer(std::chrono::high_resolution_clock::now());
