/******************************************************************************
 Copyright (c) 2016-17 Leandro T. C. Melo (ltcmelo@gmail.com)

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 for more details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *****************************************************************************/

#ifndef PSYCHE_COMPILER_FACADE_H__
#define PSYCHE_COMPILER_FACADE_H__

#include "Config.h"
#include <string>

namespace psyche {

class PSYCHEC_API CompilerFacade
{
public:
    CompilerFacade(const std::string& nativeCC);

    std::string preprocessSource(const std::string& source);
    std::string preprocessFile(const std::string& filePath);

private:
    std::string nativeCC_;
};

} // namespace psyche

#endif
