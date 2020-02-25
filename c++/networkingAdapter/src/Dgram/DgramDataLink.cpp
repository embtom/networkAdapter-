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

//******************************************************************************
// Header

#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <error_msg.hpp>
#include <Dgram/DgramDataLink.hpp>

using namespace EtNet;

//*****************************************************************************
// Method definitions "CDgramDataLink"

CDgramDataLink::CDgramDataLink(int socketFd) noexcept :
    m_socketFd(socketFd),
    m_peerAdr({CIpAddress(), 0})
{ }

CDgramDataLink::CDgramDataLink(int socketFd, const SPeerAddr &rPeerAddr) noexcept :
    m_socketFd(socketFd),
    m_peerAdr(rPeerAddr)
{ }

CDgramDataLink::CDgramDataLink(CDgramDataLink &&rhs) noexcept
{
    std::swap(m_socketFd, rhs.m_socketFd);
    std::swap(m_peerAdr, rhs.m_peerAdr);
}

CDgramDataLink& CDgramDataLink::operator=(CDgramDataLink&& rhs) noexcept
{
    std::swap(m_socketFd, rhs.m_socketFd);
    std::swap(m_peerAdr, rhs.m_peerAdr);
    return *this;
}

CDgramDataLink::~CDgramDataLink() noexcept = default;

void CDgramDataLink::send(const utils::span<char>& rSpanTx)
{
    sendTo(m_peerAdr, rSpanTx);
}

void CDgramDataLink::sendTo(const SPeerAddr& rClientAddr, const utils::span<char>& rSpanTx)
{
    sockaddr_in clAddr{};
    sockaddr_in6 clAddr6{};
    sockaddr* claddr;
    socklen_t claddrLen;

    if (rClientAddr.Ip.is_v4())
    {
        clAddr.sin_family       = AF_INET;
        clAddr.sin_port         = htons(rClientAddr.Port);
        std::memcpy(&clAddr.sin_addr, rClientAddr.Ip.to_v4(), sizeof(in_addr));
        claddr = (sockaddr*)&clAddr;
        claddrLen = sizeof(sockaddr_in);
    }
    else if (rClientAddr.Ip.is_v6())
    {
        clAddr6.sin6_family      = AF_INET6;
        clAddr6.sin6_port        = htons(rClientAddr.Port);
        std::memcpy(&clAddr6.sin6_addr, rClientAddr.Ip.to_v6(), sizeof(in6_addr));
        claddr = (sockaddr*)&clAddr6;
        claddrLen = sizeof(sockaddr_in6);
    }
    else {
        throw std::logic_error(utils::buildErrorMessage("CDgramServer::", __func__, " : No valid Ip to connect"));
    }

    std::size_t dataWritten = 0;
    while(dataWritten < rSpanTx.size_bytes())
    {
        std::size_t put = ::sendto(m_socketFd, rSpanTx.data() + dataWritten, rSpanTx.size_bytes() - dataWritten, 0, claddr, claddrLen);
        if (put == static_cast<std::size_t>(-1))
        {
            switch(errno)
            {
                case EINVAL:     [[fallthrough]];
                case EBADF:      [[fallthrough]];
                case ECONNRESET: [[fallthrough]];
                case ENXIO:      [[fallthrough]];
                case EPIPE:
                {
                    // Fatal error
                    throw std::domain_error(utils::buildErrorMessage("DataSocket::", __func__, ": write: critical error: ", strerror(errno)));
                }
                case EDQUOT:     [[fallthrough]];
                case EFBIG:      [[fallthrough]];
                case EIO:        [[fallthrough]];
                case ENETDOWN:   [[fallthrough]];
                case ENETUNREACH:[[fallthrough]];
                case ENOSPC:
                {
                    // Resource acquisition failure or device error
                    throw std::runtime_error(utils::buildErrorMessage("DataSocket::", __func__, ": write: resource failure: ", strerror(errno)));
                }
                case EINTR:      [[fallthrough]];
                        // TODO: Check for user interrupt flags.
                        //       Beyond the scope of this project
                        //       so continue normal operations.
                case EAGAIN:
                {
                    // Temporary Error, retry the read.
                    continue;
                }
                default:
                {
                    throw std::runtime_error(utils::buildErrorMessage("DataSocket::", __func__, ": write: returned -1: ", strerror(errno)));
                }
            }
        }
        dataWritten += put;
    }
    return;
}

void CDgramDataLink::reciveFrom(utils::span<char>& rSpanRx, CallbackReciveFrom scanForEnd)
{
    union e
    {
        sockaddr_storage  common;
        sockaddr_in       sin;
        sockaddr_in6      sin6;
    } peerAdr;
    socklen_t addr_size = sizeof(peerAdr);

    SPeerAddr peerAddress;
    auto toCIpAddress = [&peerAddress] (e& peerAdr)
    {
        switch (peerAdr.common.ss_family)
        {
            case AF_INET:
            {
                peerAddress.Ip = std::move(CIpAddress(peerAdr.sin.sin_addr));
                peerAddress.Port = ntohs(peerAdr.sin.sin_port);
                break;
            }
            case AF_INET6:
            {
                if (IN6_IS_ADDR_V4MAPPED(&peerAdr.sin6.sin6_addr)) {
                    in_addr ip4;
                    std::memcpy(&ip4, &peerAdr.sin6.sin6_addr.__in6_u.__u6_addr8[12], sizeof(in_addr));
                    peerAddress.Ip = std::move(CIpAddress(ip4));
                }
                else {
                    peerAddress.Ip = std::move(CIpAddress(peerAdr.sin6.sin6_addr));
                }
                peerAddress.Port = ntohs(peerAdr.sin6.sin6_port);
                break;
            }
        }
    };
    char* readBuffer = rSpanRx.data();
    std::size_t dataRead  = 0;

    while(dataRead < rSpanRx.size_bytes())
    {
        // The inner loop handles interactions with the socket.
        std::size_t get = ::recvfrom(m_socketFd, readBuffer + dataRead, rSpanRx.size_bytes() - dataRead, 0, (sockaddr*)&peerAdr, &addr_size);
        if (get == static_cast<std::size_t>(-1))
        {
            switch(errno)
            {
                case EBADF:     [[fallthrough]];
                case EFAULT:    [[fallthrough]];
                case EINVAL:    [[fallthrough]];
                case ENXIO:
                {
                    // Fatal error. Programming bug
                    throw std::domain_error(utils::buildErrorMessage("DataSocket::", __func__, ": read: critical error: ", strerror(errno)));
                }

                case EIO:       [[fallthrough]];
                case ENOBUFS:   [[fallthrough]];
                case ENOMEM:
                {
                   // Resource acquisition failure or device error
                    throw std::runtime_error(utils::buildErrorMessage("DataSocket::", __func__, ": read: resource failure: ", strerror(errno)));
                }
                case EINTR:     [[fallthrough]];
                    // TODO: Check for user interrupt flags.
                    //       Beyond the scope of this project
                    //       so continue normal operations.
                case ETIMEDOUT: [[fallthrough]];
                case EAGAIN:
                {
                    // Temporary error, retry
                    continue;
                }
                case ECONNRESET:[[fallthrough]];
                case ENOTCONN:
                {
                    // Connection broken.
                    // Return the data we have available and exit
                    // as if the connection was closed correctly.
                    get = 0;
                    break;
                }
                default:
                {
                    throw std::runtime_error(utils::buildErrorMessage("DataSocket::", __func__, ": read: returned -1: ", strerror(errno)));
                }
            }
        }
        if (get == 0)
        {
            break;
        }
        dataRead += get;

        toCIpAddress(peerAdr);
        if (scanForEnd(peerAddress, utils::span<char>(readBuffer, dataRead)))
        {
            break;
        }
    }

    rSpanRx = utils::span<char>(readBuffer, dataRead);
}
