// Copyright (c) 2024 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSD-2-Clause

/**
 * @file
 *
 * This file contains a subset of code from an application that generates
 * the ideal layout of clavichord strings.
 *
 */

//[ clavichord-example

#include <array>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#if ZEEM_CXX_MODULE
import zeem;
#else
#include <zeem/zeem.hpp>
#endif

enum class BindingType
{
	Swedish,
	German
};

enum class NoteName
{
	C,
	C_sharp,
	D,
	E_flat,
	E,
	F,
	F_sharp,
	G,
	G_sharp,
	A,
	B_flat,
	B
};

struct Note
{
	NoteName name;
	float pitch;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
        ar & zeem::make_attribute_nvp("id", name)
           & zeem::make_attribute_nvp("f", pitch);
		// clang-format on
	}
};

struct Tuning
{
	float A_frequency{};
	std::array<Note, 12> notes{};

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
        ar & zeem::make_attribute_nvp("a", A_frequency)
           & zeem::make_element_nvp("noot", notes);
		// clang-format on
	}
};

struct Binding
{
	BindingType type{};
	std::string start;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
        ar & zeem::make_attribute_nvp("schema", type)
           & zeem::make_attribute_nvp("vanaf", start);
		// clang-format on
	}
};

struct Stringing
{
	float angle{};
	float stress{};
	std::optional<Binding> binding;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
        ar & zeem::make_attribute_nvp("hoek", angle)
           & zeem::make_attribute_nvp("ideale-stress", stress)
           & zeem::make_element_nvp("gebonden", binding);
		// clang-format on
	}
};

struct ClavichordSettings
{
	std::string name;
	std::string description;
	Tuning tuning{};
	Stringing strings;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
        ar & zeem::make_element_nvp("naam", name)
           & zeem::make_element_nvp("omschrijving", description)
           & zeem::make_element_nvp("stemming", tuning)
           & zeem::make_element_nvp("snaren", strings);
		// clang-format on
	}
};

int main()
{
	zeem::value_serializer<BindingType>::init({
		// clang-format off
        { BindingType::German, "german" },
        { BindingType::Swedish, "swedish" }
		// clang-format on
	});

	zeem::value_serializer<NoteName>::init({
		// clang-format off
        { NoteName::C, "c" },
        { NoteName::C_sharp, "c#" },
        { NoteName::D, "d" },
        { NoteName::E_flat, "eb" },
        { NoteName::E, "e" },
        { NoteName::F, "f" },
        { NoteName::F_sharp, "f#" },
        { NoteName::G, "g" },
        { NoteName::G_sharp, "g#" },
        { NoteName::A, "a" },
        { NoteName::B_flat, "bb" },
        { NoteName::B, "b" }
		// clang-format on
	});

	ClavichordSettings cs;

	try
	{
		zeem::document doc;
		doc.set_validating(true);

		std::ifstream f("clavichord-v2.xml");
		f >> doc;

		from_xml(doc, "data", cs);

		// And now do something useful with the data in cs
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return 0;
}

//]
