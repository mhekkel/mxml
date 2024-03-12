//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

//[ serialisation
#include "mxml.hpp"

#include <fstream>
#include <vector>

struct Person
{
    std::string firstname;
    std::string lastname;

    /* A struct we want to serialize needs a `serialize` method */
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & mxml::make_element_nvp("firstname", firstname)
           & mxml::make_element_nvp("lastname", lastname);
    }
};

int main()
{
    /* Read in a text document containing XML and parse it into a document object */
    std::ifstream file("test.xml");
    mxml::document doc(file);
    
    std::vector<Person> persons;
    /* Deserialize all persons into an array */
    from_xml(doc, "persons", persons);

    doc.clear();

    /* Serialize all persons back into an XML document again */
    to_xml(doc, "persons", persons);

    return 0;
}
//]
