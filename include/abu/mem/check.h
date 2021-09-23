// Copyright 2021 Francois Chabot

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ABU_MEM_CHECK_H_INCLUDED
#define ABU_MEM_CHECK_H_INCLUDED

#include "abu/base.h"

#ifndef ABU_MEM_ASSUMPTIONS
#define ABU_MEM_ASSUMPTIONS assume  // NOLINT
#endif

#ifndef ABU_MEM_PRECONDITIONS
#ifdef NDEBUG
#define ABU_MEM_PRECONDITIONS assume  // NOLINT
#else
#define ABU_MEM_PRECONDITIONS verify  // NOLINT
#endif
#endif

namespace abu::mem {

inline constexpr void assume(bool condition,
                             std::string_view msg = {},
                             abu::base::source_location location =
                                 abu::base::source_location::current()) {
  return abu::base::check(
      abu::base::ABU_MEM_ASSUMPTIONS, condition, msg, location);
}

inline constexpr void precondition(bool condition,
                                   std::string_view msg = {},
                                   abu::base::source_location location =
                                       abu::base::source_location::current()) {
  return abu::base::check(
      abu::base::ABU_MEM_PRECONDITIONS, condition, msg, location);
}
}  // namespace abu::mem

#endif
