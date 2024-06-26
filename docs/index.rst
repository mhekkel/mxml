Introduction
============

This is a feature complete XML library containing a validating parser as well as a modern C++ API for the data structures. It also supports serializing custom data structures. 

The core of this library is a validating XML parser with DTD processing and all. On top of this are implemented an API for manipulating XML data in a DOM like fashion and a serialization API. As a bonus there's also an XPath implementation, albeit this is limited to XPath 1.0.

This XML library was extracted from `libzeep <https://github.com/mhekkel/libzeep>`_ since having a separate and simple XML library is more convenient. The API is unfortunately no longer compatible since the goal was to be more standards compliant. E.g., :cpp:class:`mxml::element` should now be a complete `Sequence Container <https://en.cppreference.com/w/cpp/named_req/SequenceContainer>`_

The DOM API
-----------

MXML uses a modern C++ way of accessing and manipulating data. Look at the following code to get an idea how this works.

.. literalinclude:: ../examples/synopsis-xml.cpp
    :language: c++
    :start-after: //[ synopsis_xml_main
    :end-before: //]

XML nodes
^^^^^^^^^

The class :cpp:class:`mxml::node` is the base class for all classes in the DOM API. The class is not copy constructable and subclasses use move semantics to offer a simple API while still being memory and performance efficient. Nodes can have siblings and a parent but no children.

The class :cpp:class:`mxml::element` is the main class, it implements a full XML node with child nodes and attributes. The children are stored as a linked list and same goes for the attributes.

The class :cpp:class:`mxml::text` contains the text between XML elements. A :cpp:class:`mxml::cdata` class is derived from :cpp:class:`mxml::text` and other possible child nodes for an XML element are :cpp:class:`mxml::processing_instruction` and :cpp:class:`mxml::comment`.

XML elements also contain attributes, stored in the :cpp:class:`mxml::attribute` class. Namespace information is stored in these attributes as well. Attributes support structured binding, so the following works:

.. code-block:: cpp

    mxml::attribute a("x", "1");
    auto& [name, value] = a; // name == "x", value == "1"

Input and output
--------------------------------------

The class :cpp:class:`mxml::document` looks very much like a :cpp:class:`mxml::element` but can only contain one child and no attributes. This class can load from and write to files.

streaming I/O
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use std::iostream to read and write :cpp:class:`mxml::document` objects. Reading is as simple as:

.. code-block:: cpp

    mxml::document doc;
    std::cin >> doc;

Writing is just as simple. A warning though, round trip fidelity is not guaranteed. There are a few issues with that. First of all, the default is to replace CDATA sections in a file with their content. If this is not the desired behaviour you can call :cpp::func:`mxml::document::set_preserve_cdata` with argument `true`.

Another issue is that text nodes containing only white space are present in documents read from disk while these are absent by default in documents created on the fly. When writing out XML using `iostream` you can specify to wrap and indent a document. But if the document was read in, the result will have extraneous spacing.

Specifying indentation is BTW done like this:

.. code-block:: cpp

    std::cout << std::setw(2) << doc;

That will indent with two spaces for each level.

validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This will not validate the XML using the DTD by default. If you do want to validate and process the DTD, you have to specify where to find this DTD and other external entities. You can either use :cpp::func:`mxml::document::set_base_dir` or you can specify an entity_loader using :cpp::func:`mxml::document::set_entity_loader`

As an example, take the following DTD file

.. code-block:: xml

    <!ELEMENT foo (bar)>
    <!ELEMENT bar (#PCDATA)>
    <!ENTITY hello "Hello, world!">

And an XML document containing

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <!DOCTYPE foo SYSTEM "sample.dtd">
    <foo>
    <bar>&hello;</bar>
    </foo>

When we want to see the `&hello;` entity replaced with `'Hello, world!'` as specified in the DTD, we need to provide a way to load this DTD. To do this, look at the following code. Of course, in this example a simple call to :cpp::func:`mxml::document::set_base_dir` would have been sufficient.

.. literalinclude:: ../examples/validating-xml-sample.cpp
    :language: c++
    :start-after: //[ xml_validation_sample
    :end-before: //]

Serialization
--------------------------------------

An alternative way to read/write XML files is using serialization. To do this, we first construct a structure called Person. We add a templated function to this struct just like in `boost::serialize` and then we can read the file.

.. literalinclude:: ../examples/serialize-xml.cpp
    :language: c++
    :start-after: //[ serialisation
    :end-before: //]

attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Suppose you want to serialize a value into a XML attribute, you would have to replace `mxml::make_element_nvp` with `mxml::make_attribute_nvp`.

custom types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

What happens during serialization is deconstruction of structured data types into parts that can be converted into text strings. For this final conversion there are mxml::value_serializer helper classes. mxml::value_serializer is a template and specializations for the default types are given in `<mxml/serializer.ixx>`. You can create your own specializations for this class for custom data types, look at the one for `std::chrono::system_clock::time_point` for inspiration.

enums
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For conversion of enum's you can use the `mxml::value_serializer` specialization for enums:

.. code-block:: cpp

    enum class MyEnum { FOO, BAR };
    mxml::value_serializer<MyEnum>::init({
        { MyEnum::FOO, "foo" },
        { MyEnum::BAR, "bar" }
    });

XPath 1.0
--------------------------------------

MXML comes with a [XPath 1.0](http://www.w3.org/TR/xpath/) implementation. You can use this to locate elements in a DOM tree easily. For a complete description of the XPath specification you should read the documentation at e.g. http://www.w3.org/TR/xpath/ or https://www.w3schools.com/xml/xpath_intro.asp.

The way it works in MXML is that you can call `find()` on an :cpp:class:`mxml::element` object and it will return a `mxml::element_set` object which is actually a `std::list` of :cpp:class:`mxml::element` pointers of the elements that conform to the specification in XPath passed as parameter to `find()`. An alternative method `find_first()` can be used to return only the first element.

An example where we look for the first person in our test file with the lastname Jones:

.. code-block:: cpp

    mxml::element* jones = doc.child()->find_first("//person[lastname='Jones']");

variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

XPath constructs can reference variables. As an example, suppose you need to find nodes in a special XML Namespace but you do not want to find out what the prefix of this Namespace is, you could do something like this:

.. literalinclude:: ../examples/xpath-sample.cpp
    :language: c++
    :start-after: //[ xpath_sample
    :end-before: //]

.. note::

    Please note that the evaluation of an XPath returns pointers to XML nodes. Of course these are only valid as long as you do not modify the the document in which they are contained.

Real world example
--------------------

A real world example is given in the clavichord-example application. It uses a subset of configuration data to calculate the ideal layout of strings for a clavichord.

Start by looking at the following DTD files:

.. literalinclude:: ../examples/clavichord-stringing.dtd
    :language: xml

And 

.. literalinclude:: ../examples/werckmeister.xml
    :language: xml

These files define what needs to go into a configuration file for our program. A sample configuration file may then look like this:

.. literalinclude:: ../examples/clavichord-v2.xml
    :language: xml

And the application code to read this data looks like this:

.. literalinclude:: ../examples/clavichord-example.cpp
    :language: c++
    :start-after: //[ clavichord-example
    :end-before: //]



.. toctree::
   :maxdepth: 2
   :caption: Contents
   
   self
   api/library_root.rst
   genindex

