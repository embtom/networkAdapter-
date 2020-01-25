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

#ifndef _ENDIANCONVERTER_H_
#define _ENDIANCONVERTER_H_

#include <string>
#include <type_traits>
#include <array>
#include <EndianMeta.h>
#include <EndianMembers.h>
#include <template_helpers.h>

namespace EtEndian
{

enum class EConvertMode
{
    NET_ORDER,
    HOST_ORDER
};

template<typename T>
class ConverterFunc
{

public:
    explicit ConverterFunc(EConvertMode mode, T&& rObj) noexcept :
        m_ConvertMode(mode),
        m_initialObj(std::forward<T>(rObj)) 
    { };

    template<typename Member,
             typename = std::enable_if_t<std::is_arithmetic<EtEndian::get_member_type<Member>>::value>
    >
    void operator()(const Member& member) noexcept
    {
        using element_t = EtEndian::get_member_type<Member>;
        const element_t& source = member.getConstRef(m_initialObj);  

        switch (m_ConvertMode)
        {
            case EConvertMode::NET_ORDER:
            {
                member.set(m_convertedObj, host_to_network(source));
                break;
            }
            case EConvertMode::HOST_ORDER:
            {
                member.set(m_convertedObj, network_to_host(source));
                break;
            } 
        }
    }

    template<typename Member,
                typename = std::enable_if_t<detail::is_array<EtEndian::get_member_type<Member>>::value>,
                typename = void
    >   
    void operator()(const Member& member) noexcept
    {
        using array_t = EtEndian::get_member_type<Member>;
        using array_element_t = std::decay_t<decltype(std::declval<array_t>()[0])>;

        static_assert(std::is_arithmetic<array_element_t>::value, "No arithmetic type");
        
        constexpr std::size_t N = decltype(std::declval<array_t>()){}.size();

        const array_t& sourceArray= member.getConstRef(m_initialObj);
        array_t& destArray= member.getRef(m_convertedObj);

        switch (m_ConvertMode)
        {
            case EConvertMode::NET_ORDER:
            {
                std::transform(sourceArray.begin(), sourceArray.end(), destArray.begin(), 
                    [](const auto& a) { return host_to_network(a); });
                break;
            }
            case EConvertMode::HOST_ORDER:
            {
                std::transform(sourceArray.begin(), sourceArray.end(), destArray.begin(), 
                    [](const auto& a) { return network_to_host(a); });
                break;
            }    
        }
        
    }

    const T& value() const noexcept
    {
        return m_initialObj;
    }

    const T& converted() const noexcept
    {
        return m_convertedObj;
    }

private:
    const EConvertMode  m_ConvertMode;
    const T             m_initialObj;
    T                   m_convertedObj;
};

}

#endif // _ENDIANCONVERTER_H_
