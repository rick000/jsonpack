/*
       _  _____  ____  _   _ _____        _____ _  __
      | |/ ____|/ __ \| \ | |  __ \ /\   / ____| |/ /
      | | (___ | |  | |  \| | |__) /  \ | |    | ' / 
  _   | |\___ \| |  | | . ` |  ___/ /\ \| |    |  <  
 | |__| |____) | |__| | |\  | |  / ____ \ |____| . \ 
  \____/|_____/ \____/|_| \_|_| /_/    \_\_____|_|\_\ 

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2013-2022 Rick Han <rick.han@yahoo.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef INCLUDE_JSON_TRANSFORMER_HPP_
#define INCLUDE_JSON_TRANSFORMER_HPP_

#include <tuple>
#include <type_traits>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json; 


template<typename T>
struct jsonpack_register_type; 

#define REGISTER_JSONPACK_TYPE(Type, ...)                               \
template <>                                                             \
struct jsonpack_register_type<Type>                                     \
{                                                                       \
    using cpp_type = Type;                                              \
    template <typename I = void>                                        \
    static inline constexpr auto get() -> decltype(std::make_tuple(__VA_ARGS__))\
    {                                                                   \
        return std::make_tuple(__VA_ARGS__);                            \
    }                                                                   \
};                                                                      \


#define REGISTER_JSONPACK_FIELD(Field, JsonName)                        \
    std::make_tuple(&cpp_type::Field, JsonName)                         \


#define REGISTER_JSONPACK_FIELD_FUNC(Field, JsonName, Func)             \
    std::make_tuple(&cpp_type::Field, JsonName, Func)                   \


template <typename T, typename ENABLED = void>
struct is_jsonpack_type : std::false_type {};

template <typename T>
struct is_jsonpack_type<T, std::void_t<typename jsonpack_register_type<std::decay_t<T>>::cpp_type>> : std::true_type {};

namespace jsonpack {

template<typename T>
inline constexpr void json_to_type(const json& json, T& out_type);
template<typename T>
inline constexpr void json_to_type(const json& json, std::vector<T>& out_type);
template <typename T>
inline constexpr void type_to_json(const T& in_type, json& out_json);
template <typename T>
inline constexpr void type_to_json(const std::vector<T>& in_type, json& out_json);

template<typename T>
inline constexpr void json_value_to_field(const json& json, const std::string& name, T& field)
{
    if (json.count(name) == 0) return;
    const auto& v = json[name];
    if (v.is_discarded() || v.is_null()) return;
    if constexpr (is_jsonpack_type<T>::value)
    {
        json_to_type(v, field);
    }
    else
    {   
        field = v.get<T>();
    }
}


template <typename T>
inline constexpr void json_value_to_field(const json& json, const std::string& name, std::vector<T>& field)
{
    if (json.count(name) == 0 || json[name].is_array() == false)
    {
        return;
    }

    const auto& v = json[name];
    if constexpr (is_jsonpack_type<T>::value)
    {
        for (int i = 0; i < v.size(); ++i) 
        {
            T tmp_value;
            json_to_type(v.at(i), tmp_value);
            field.emplace_back(tmp_value);
        }
    }
    else
    {
        for (int i = 0; i < v.size(); ++i) 
        {
            T tmp_value = v.at(i).get<T>();
            field.emplace_back(tmp_value);
        }
    }
}

template <typename TypeInfo, typename Struct>
inline constexpr void fill_json_into_cpp(TypeInfo&& type_info, Struct& value, const json& json)
{
    if (json.is_discarded() || json.is_null()) return;
    auto& field = value.*(std::get<0>(type_info));
    auto& name = std::get<1>(type_info);

    if constexpr (std::tuple_size_v<std::decay_t<TypeInfo>> == 3) 
    {
        std::get<2>(type_info)(json, name, field);
    }   
    else
    {
        json_value_to_field(json, name, field);
    }
}


template <typename T>
inline constexpr void json_to_type(const json& input, T& out_type)
{
    static_assert(is_jsonpack_type<T>::value);
    auto t = jsonpack_register_type<std::decay_t<T>>::get();
    std::apply([&out_type, &input = std::as_const(input)] (auto& ...args)
        {
            (fill_json_into_cpp(args, out_type, input), ...);
        }, t);
}

template <typename T>
inline constexpr void json_to_type(const json& json, std::vector<T>& out_type)
{
    static_assert(is_jsonpack_type<T>::value);
    out_type = {};
    if (json.is_array()) 
    {
        for (int i = 0; i < json.size(); ++i)
        {
            T tmp_value;
            json_to_type(json.at(i), tmp_value);
            out_type.emplace_back(tmp_value);
        }
    }
}


template <typename T>
inline constexpr void field_to_json(json& json, const std::string& name, const T& field)
{
    auto& v = json[name];
    if constexpr (is_jsonpack_type<T>::value)
    {
        type_to_json(field, v);
    }
    else
    {   
        v = field;
    }
}

template <typename T>
inline constexpr void field_to_json(json& out_data, const std::string& name, const std::vector<T>& field)
{
    json array_data = {};
    if constexpr (is_jsonpack_type<T>::value)
    {
        for (const auto& f : field)
        {
            json v;
            type_to_json(f, v);
            array_data.push_back(v);
        }   
    }
    else
    {
        for (const auto& f : field)
        {
            json v = field;
            array_data.push_back(v);
        }
    }
    out_data[name] = array_data;
}

template <typename TypeInfo, typename Struct>
inline constexpr void write_cpp_into_json(TypeInfo&& type_info, const Struct& value, json& json)
{
    const auto& field = value.*(std::get<0>(type_info));
    auto& name = std::get<1>(type_info);

    if constexpr (std::tuple_size_v<std::decay_t<TypeInfo>> == 3) 
    {
        std::get<2>(type_info)(json, name, field);
    } 
    else
    {
        field_to_json(json, name, field);
    }
}

template <typename T>
inline constexpr void type_to_json(const T& in_type, json& out_json)
{
    static_assert(is_jsonpack_type<T>::value);
    out_json = {};
    auto t = jsonpack_register_type<std::decay_t<T>>::get();
    std::apply([&out_json, &in_type = std::as_const(in_type)] (auto& ...args)
        {
            (write_cpp_into_json(args, in_type, out_json), ...);
        }, t);
}

template <typename T>
inline constexpr void type_to_json(const std::vector<T>& in_type, json& out_json)
{
    static_assert(is_jsonpack_type<T>::value);
    out_json = {};
    for (const auto& one : in_type)
    {
        json element;
        type_to_json(one, element);
        out_json.push_back(element);
    }
}

} // namespace jsonpack

#endif // INCLUDE_JSON_TRANSFORMER_HPP_