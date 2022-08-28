#pragma once
#include <vector>
#include <tuple>
#include <type_traits>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <functional>

namespace SRefl
{
	template<typename T>
	struct TypeInfo { using _Type = T; };

	template<typename T>
	struct EnumInfo { using _Type = T; };

	template<typename Class>
    struct ClassImpl
    {
        using _Type = Class;

        const char* Name;
    };

    template<typename Class>
    constexpr auto _classref(const char* name)
    {
        return ClassImpl<Class>{name};
    }

	template<typename Class, typename T>
	struct FieldImpl
	{
		constexpr FieldImpl(T Class::* member, const char* name) : MemberPtr{ member }, Name{ name } {}

		using _Type = T;
		using _Class = Class;

		T Class::*	MemberPtr;
		const char*	Name;
	};

	template<typename Class, typename T>
	constexpr auto _field(T Class::* member, const char* name)
	{
		return FieldImpl<Class, T>{member, name};
	}

	template<typename Class>
	struct EnumImpl
	{
		constexpr EnumImpl(Class value, const char* name) : Value{ value }, Name{ name } {}
		using _Class = Class;

		Class Value;
		const char* Name;
	};

	template<typename Class>
	constexpr auto _enumTerm(Class value, const char* name)
	{
		return EnumImpl<Class>{value, name};
	}

	template<typename T, typename = void>
	struct has_fields : public std::false_type {};

	template<typename T>
	struct has_fields<T, std::void_t<decltype(TypeInfo<T>::_FIELDLIST())>> : std::true_type {};

	template<typename T>
	constexpr bool has_fields_v = has_fields<T>::value;

	template<typename T, typename = void>
	struct has_bases : public std::false_type {};

	template<typename T>
	struct has_bases<T, std::void_t<decltype(TypeInfo<T>::_BASELIST())>> : std::true_type {};

	template<typename T>
	constexpr bool has_bases_v = has_bases<T>::value;


	template <typename T, T... S, typename F>
	inline constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f)
	{
		(static_cast<void>(f(std::integral_constant<T, S>{})), ...);
	}

	template<typename T, typename F, std::enable_if_t<SRefl::has_fields_v<T>, nullptr_t> = nullptr>
	inline constexpr void ForEachField(F&& func)
	{
		constexpr auto nTypes = std::tuple_size<decltype(TypeInfo<T>::_FIELDLIST())>::value;
		for_sequence(std::make_index_sequence<nTypes>{}, [&](auto i) {
			// get the field
			constexpr auto field = std::get<i>(TypeInfo<T>::_FIELDLIST());

			// apply function to the field
			func(field);
		});
	}

	template<typename T, typename F, std::enable_if_t<SRefl::has_bases_v<T>, nullptr_t> = nullptr>
	inline constexpr void ForEachBase(F&& func)
	{
		constexpr auto nTypes = std::tuple_size_v<decltype(TypeInfo<T>::_BASELIST())>;
		for_sequence(std::make_index_sequence<nTypes>{}, [&](auto i) {
			// get the base type
			constexpr auto base = std::get<i>(TypeInfo<T>::_BASELIST());

			// apply function to the base type
			func(base);
		});
	}

	template<typename T, typename F, std::enable_if_t<std::is_enum_v<T>, nullptr_t> = nullptr>
	inline constexpr void ForEachEnumItem(F&& func)
	{
		constexpr auto nTypes = std::tuple_size<decltype(EnumInfo<T>::_ENUMLIST())>::value;
		for_sequence(std::make_index_sequence<nTypes>{}, [&](auto i) {
			// get the field
			constexpr auto item = std::get<i>(EnumInfo<T>::_ENUMLIST());

			// apply function to the field
			func(item);
			});
	}

#define SREFL_TYPE_HEADER(Type) friend struct SRefl::TypeInfo<Type>
#define SREFL_TYPEINFO_HEADER(Type) using _Type = Type;
#define SREFL_FIELD(member) SRefl::_field(&_Type::member, #member)
#define SREFL_FIELDLIST(Type, ...) constexpr static auto _FIELDLIST()\
{\
	return std::make_tuple(__VA_ARGS__);\
}\

#define SREFL_BASECLASS(type) SRefl::_classref<type>(#type)

#define SREFL_ENUM_TERM(member) SRefl::_enumTerm(_Type::member, #member)


#define SREFL_ENUM(member) SRefl::_field(&_Type::member, #member)
#define ENUM_KEY(key) case ENUM_TYPE::key: return #key;
#define ENUM_VALUE(key) {#key, ENUM_TYPE::key},
#define GENERATE_ENUM_MAPPING(T, LISTFUNC)\
    static std::string enum_to_string(T v)\
    {\
        using ENUM_TYPE = T;\
        switch (v)\
        {\
        LISTFUNC(ENUM_KEY)\
        default:\
            break;\
        }\
        throw std::logic_error("Invalid enum type");\
    }\
	static T string_to_enum(const std::string& str)\
	{\
        using ENUM_TYPE = T;\
		static std::map<std::string, ENUM_TYPE> stringmap = {\
			LISTFUNC(ENUM_VALUE)\
		};\
		std::map<std::string, ENUM_TYPE>::iterator it;\
		if ((it = stringmap.find(str)) != stringmap.end())\
		{\
			return it->second;\
		}\
		throw std::invalid_argument("Invalid enum type");\
	}
};