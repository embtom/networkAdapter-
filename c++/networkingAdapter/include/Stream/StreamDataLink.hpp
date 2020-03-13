/*
 * This file is part of the EMBTOM project
 * Copyright (c) 2018-2019 Thomas Willetal
 * (https://github.com/embtom)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _STREAMDATALINK_H_
#define _STREAMDATALINK_H_

//******************************************************************************
// Header

#include <stdint.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <span.h>

namespace EtNet
{

constexpr auto defaultOneRead = [](utils::span<char> rx){ return true; };

class CStreamDataLinkPrivate;
//*****************************************************************************
//! \brief CStreamDataLink
//!
class CStreamDataLink
{
public:
    enum class ERet
    {
        OK,
        UNBLOCK
    };

    using CallbackReceive = std::function<bool (utils::span<char> rx)>;

    CStreamDataLink() noexcept                                 = default;
    CStreamDataLink(CStreamDataLink const&)                    = default;
    CStreamDataLink& operator=(CStreamDataLink const&)         = default;
    virtual ~CStreamDataLink() noexcept;

    CStreamDataLink(int socketFd) noexcept;
    CStreamDataLink(CStreamDataLink &&rhs) noexcept;
    CStreamDataLink& operator=(CStreamDataLink&& rhs) noexcept;

    void send(const utils::span<char>& rTxSpan);

    bool unblockRecive() noexcept;
    CStreamDataLink::ERet recive(utils::span<char>& rRxSpan, CallbackReceive scanForEnd = defaultOneRead);

private:
    std::shared_ptr<CStreamDataLinkPrivate> m_pPrivate;
};

}

#endif /* _STREAMDATALINK_H_ */
