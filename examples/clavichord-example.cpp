/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 * This file contains a subset of code from an application that generates
 * the ideal layout of clavichord strings.
 *
 */

//[ clavichord-example

#include "mxml.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

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
    void serialize(Archive &ar, unsigned long version)
    {
        // clang-format off
        ar & mxml::make_attribute_nvp("id", name)
           & mxml::make_attribute_nvp("f", pitch);
        // clang-format on
    }
};

struct Tuning
{
    float A_frequency;
    std::array<Note, 12> notes;

    template <typename Archive>
    void serialize(Archive &ar, unsigned long version)
    {
        // clang-format off
        ar & mxml::make_attribute_nvp("a", A_frequency)
           & mxml::make_element_nvp("noot", notes);
        // clang-format on
    }
};

struct Binding
{
    BindingType type;
    std::string start;

    template <typename Archive>
    void serialize(Archive &ar, unsigned long version)
    {
        // clang-format off
        ar & mxml::make_attribute_nvp("schema", type)
           & mxml::make_attribute_nvp("vanaf", start);
        // clang-format on
    }
};

struct Stringing
{
    float angle;
    float stress;
    std::optional<Binding> binding;

    template <typename Archive>
    void serialize(Archive &ar, unsigned long version)
    {
        // clang-format off
        ar & mxml::make_attribute_nvp("hoek", angle)
           & mxml::make_attribute_nvp("ideale-stress", stress)
           & mxml::make_element_nvp("gebonden", binding);
        // clang-format on
    }
};

struct ClavichordSettings
{
    std::string name;
    std::string description;
    Tuning tuning;
    Stringing strings;

    template<typename Archive>
    void serialize(Archive &ar, unsigned long version)
    {
        // clang-format off
        ar & mxml::make_element_nvp("naam", name)
           & mxml::make_element_nvp("omschrijving", description)
           & mxml::make_element_nvp("stemming", tuning)
           & mxml::make_element_nvp("snaren", strings);
        // clang-format on
    }
};

int main()
{
    mxml::value_serializer<BindingType>::init({
        // clang-format off
        { BindingType::German, "german" },
        { BindingType::Swedish, "swedish" }
        // clang-format on
    });

    mxml::value_serializer<NoteName>::init({
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
        mxml::document doc;
        doc.set_validating(true);

        std::ifstream f("clavichord-v2.xml");
        f >> doc;

        from_xml(doc, "data", cs);

        // And now do something useful with the data in cs

    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
    }

    return 0;
}

//]
