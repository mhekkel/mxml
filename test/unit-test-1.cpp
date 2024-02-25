#define CATCH_CONFIG_RUNNER

#if CATCH22
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <filesystem>

import mxml;

std::filesystem::path gTestDir = std::filesystem::current_path();

int main(int argc, char *argv[])
{
	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch2's
#if CATCH22
	using namespace Catch::clara;
#else
	// Build a new parser on top of Catch2's
	using namespace Catch::Clara;
#endif

	auto cli = session.cli()                               // Get Catch2's command line parser
	           | Opt(gTestDir, "data-dir")                 // bind variable to a new option, with a hint string
	                 ["-D"]["--data-dir"]                  // the option names it will respond to
	           ("The directory containing the data files"); // description string for the help output
	        //    | Opt(cif::VERBOSE, "verbose")["-v"]["--cif-verbose"]("Flag for cif::VERBOSE");

	// Now pass the new composite back to Catch2 so it uses that
	session.cli(cli);

	// Let Catch2 (using Clara) parse the command line
	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	// // do this now, avoids the need for installing
	// cif::add_file_resource("mmcif_pdbx.dic", gTestDir / ".." / "rsrc" / "mmcif_pdbx.dic");

	// // initialize CCD location
	// cif::add_file_resource("components.cif", gTestDir / ".." / "rsrc" / "ccd-subset.cif");

	// cif::compound_factory::instance().push_dictionary(gTestDir / "HEM.cif");

	return session.run();
}

TEST_CASE("test_1")
{
	mxml::element n("test");

	CHECK(n.name() == "test");

	SECTION("insert")
	{
		auto i1 = n.insert(n.end(), mxml::element("c1"));

		CHECK(i1->name() == "c1");
		CHECK(i1->empty());
		CHECK(i1->size() == 0);
		CHECK(n.size() == 1);
		CHECK(n.front().name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i2 = n.insert(n.end(), mxml::element("c2"));

		CHECK(i2->name() == "c2");
		CHECK(i2->empty());
		CHECK(i2->size() == 0);
		CHECK(n.size() == 2);
		CHECK(n.front().name() == "c1");
		CHECK(n.back().name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i3 = n.insert(n.begin(), mxml::element("c0"));
		CHECK(i3->name() == "c0");
		CHECK(i3->empty());
		CHECK(i3->size() == 0);
		CHECK(n.size() == 3);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c2");

		mxml::element c3("c3");
		auto i4 = n.insert(n.end(), c3);
		CHECK(i4->name() == "c3");
		CHECK(i4->empty());
		CHECK(i4->size() == 0);
		CHECK(n.size() == 4);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c3");

		for (auto &e : n)
		{
			CHECK(e.parent() == &n);
			CHECK(e.empty());
			CHECK(e.size() == 0);
		}

		for (int i = 0; auto &e : n)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		// CHECK(find(n.begin(), n.end(), i1) != n.end());

		auto n2 = n;

		CHECK(n2.size() == 4);
		CHECK(n2.front().name() == "c0");
		CHECK(n2.back().name() == "c3");
		for (auto &e : n2)
			CHECK(e.parent() == &n2);

		for (int i = 0; auto &e : n2)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		auto n3(std::move(n2));

		CHECK(n2.empty());
		CHECK(n3.size() == 4);
		CHECK(n3.front().name() == "c0");
		CHECK(n3.back().name() == "c3");
		for (auto &e : n3)
			CHECK(e.parent() == &n3);

		for (int i = 0; auto &e : n3)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		mxml::element n4;
		n4 = std::move(n3);

		CHECK(n3.empty());
		CHECK(n4.size() == 4);
		CHECK(n4.front().name() == "c0");
		CHECK(n4.back().name() == "c3");
		for (auto &e : n4)
			CHECK(e.parent() == &n4);

		for (int i = 0; auto &e : n4)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		// erase

		for (int i = 4; i > 0; --i)
		{
			n4.erase(n4.begin());
			CHECK(n4.size() == i - 1);
		}
		CHECK(n4.empty());

		for (int i = 4; i > 0; --i)
		{
			n.erase(std::prev(n.end()));
			CHECK(n.size() == i - 1);
		}
		CHECK(n.empty());


	}

	SECTION("emplace")
	{
		auto &t = n.emplace(n.end(), "c1");

		CHECK(t.name() == "c1");
		CHECK(n.size() == 1);
		CHECK(n.front().name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto &t2 = n.emplace_back("c2");

		CHECK(t2.name() == "c2");
		CHECK(n.size() == 2);
		CHECK(n.front().name() == "c1");
		CHECK(n.back().name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto &t3 = n.emplace_front("c0");
		CHECK(t3.name() == "c0");
		CHECK(n.size() == 3);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

	}





	// auto &t = n.emplace(n.end(), "c1");

	// CHECK(t.name() == "c1");
	// CHECK(n.size() == 1);
	// CHECK(n.front().name() == "c1");

	// auto &t2 = n.emplace_back("c2");

	// CHECK(t2.name() == "c2");
	// CHECK(n.size() == 2);
	// CHECK(n.front().name() == "c1");
	// CHECK(n.back().name() == "c2");

	// auto &t3 = n.emplace_front("c0");
	// CHECK(t3.name() == "c0");
	// CHECK(n.size() == 3);
	// CHECK(n.front().name() == "c0");
	// CHECK(n.back().name() == "c2");


}