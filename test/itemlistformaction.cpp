#include "itemlistformaction.h"

#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "feedlistformaction.h"
#include "itemlist.h"
#include "keymap.h"
#include "regexmanager.h"
#include "rssfeed.h"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE("OP_OPEN displays article using an external pager",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile pagerfile;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	ConfigContainer cfg;
	FilterContainer filters;

	Cache rsscache(":memory:", &cfg);
	cfg.set_configvalue("pager", "cat %f > " + pagerfile.get_path());

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_description(test_description);
	item->set_pubDate(test_pubDate);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_OPEN));

	TestHelpers::AssertArticleFileContent(pagerfile.get_path(),
		test_title,
		test_author,
		test_pubDate_str,
		test_url,
		test_description);
}

TEST_CASE("OP_PURGE_DELETED purges previously deleted items",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;
	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	SECTION("No items to purge") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_PURGE_DELETED));
		REQUIRE(feed->total_item_count() == 1);
	}

	SECTION("Deleted items are purged") {
		item->set_deleted(true);
		REQUIRE_NOTHROW(itemlist.process_op(OP_PURGE_DELETED));
		REQUIRE(feed->total_item_count() == 0);
	}
}

TEST_CASE(
	"OP_OPENBROWSER_AND_MARK passes the url to the browser and marks read",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile browserfile;

	const std::string test_url = "http://test_url";
	std::string line;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);
	itemlist.process_op(OP_OPENBROWSER_AND_MARK);
	std::ifstream browserFileStream(browserfile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line == test_url);

	REQUIRE(feed->unread_item_count() == 0);
}

TEST_CASE(
	"OP_OPENBROWSER_AND_MARK does not mark read when browser fails",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);

	const std::string test_url = "http://test_url";

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "false %u");

	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);
	itemlist.process_op(OP_OPENBROWSER_AND_MARK);

	REQUIRE(feed->unread_item_count() == 1);
}

TEST_CASE("OP_OPENINBROWSER passes the url to the browser",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile browserfile;
	const std::string test_url = "http://test_url";
	std::string line;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);
	itemlist.process_op(OP_OPENINBROWSER);
	std::ifstream browserFileStream(browserfile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line == test_url);
}

TEST_CASE("OP_OPENALLUNREADINBROWSER passes the url list to the browser",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile browserfile;
	std::unordered_set<std::string> url_set;
	const std::string test_url = "http://test_url";
	std::string line;
	int itemCount = 6;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	for (int i = 0; i < itemCount; i++) {
		std::shared_ptr<RssItem> item =
			std::make_shared<RssItem>(&rsscache);
		item->set_link(test_url + std::to_string(i));
		url_set.insert(test_url + std::to_string(i));
		item->set_unread(true);
		feed->add_item(item);
	}

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	SECTION("unread >= max-browser-tabs") {
		int maxItemsToOpen = 4;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		itemlist.process_op(OP_OPENALLUNREADINBROWSER);

		std::ifstream browserFileStream(browserfile.get_path());
		openedItemsCount = 0;
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.")
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == maxItemsToOpen);
	}

	SECTION("unread < max-browser-tabs") {
		int maxItemsToOpen = 9;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		itemlist.process_op(OP_OPENALLUNREADINBROWSER);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.")
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == itemCount);
	}
}

TEST_CASE(
	"OP_OPENALLUNREADINBROWSER_AND_MARK passes the url list to the browser "
	"and marks them read",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile browserfile;
	std::unordered_set<std::string> url_set;
	const std::string test_url = "http://test_url";
	std::string line;
	int itemCount = 6;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	for (int i = 0; i < itemCount; i++) {
		std::shared_ptr<RssItem> item =
			std::make_shared<RssItem>(&rsscache);
		item->set_link(test_url + std::to_string(i));
		url_set.insert(test_url + std::to_string(i));
		item->set_unread(true);
		feed->add_item(item);
	}

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	SECTION("unread >= max-browser-tabs") {
		int maxItemsToOpen = 4;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.")
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == maxItemsToOpen);
		REQUIRE(feed->unread_item_count() ==
			itemCount - maxItemsToOpen);
	}

	SECTION("unread < max-browser-tabs") {
		int maxItemsToOpen = 9;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.")
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == itemCount);
		REQUIRE(feed->unread_item_count() == 0);
	}
}

TEST_CASE("OP_SHOWURLS shows the article's properties", "[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;
	TestHelpers::TempFile urlFile;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_description(test_description);
	item->set_pubDate(test_pubDate);
	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);

	SECTION("with external-url-viewer") {
		feed->add_item(item);
		itemlist.set_feed(feed);
		cfg.set_configvalue(
			"external-url-viewer", "tee > " + urlFile.get_path());

		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS));

		TestHelpers::AssertArticleFileContent(urlFile.get_path(),
			test_title,
			test_author,
			test_pubDate_str,
			test_url,
			test_description);
	}

	SECTION("internal url viewer") {
		feed->add_item(item);
		itemlist.set_feed(feed);
		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS));
	}

	SECTION("no feed in formaction") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS));
	}
}

TEST_CASE("OP_BOOKMARK pipes articles url and title to bookmark-command",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;
	TestHelpers::TempFile bookmarkFile;
	std::string line;
	std::vector<std::string> bookmark_args;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string feed_title = "Feed Title";
	std::string separator = " ";
	std::string extra_arg = "extra arg";

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	feed->set_title(feed_title);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);

	feed->add_item(item);
	itemlist.set_feed(feed);

	cfg.set_configvalue(
		"bookmark-cmd", "echo > " + bookmarkFile.get_path());

	bookmark_args.push_back(extra_arg);
	REQUIRE_NOTHROW(itemlist.process_op(OP_BOOKMARK, true, &bookmark_args));

	std::ifstream browserFileStream(bookmarkFile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line ==
		test_url + separator + test_title + separator + extra_arg +
		separator + feed_title);
}

TEST_CASE("OP_EDITFLAGS arguments are added to an item's flags",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::vector<std::string> op_args;

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);

	feed->add_item(item);
	itemlist.set_feed(feed);

	SECTION("Single flag") {
		std::string flags = "G";
		op_args.push_back(flags);

		REQUIRE_NOTHROW(
			itemlist.process_op(OP_EDITFLAGS, true, &op_args));
		REQUIRE(item->flags() == flags);
	}

	SECTION("Unordered flags") {
		std::string flags = "abdefc";
		std::string ordered_flags = "abcdef";
		op_args.push_back(flags);

		REQUIRE_NOTHROW(
			itemlist.process_op(OP_EDITFLAGS, true, &op_args));
		REQUIRE(item->flags() == ordered_flags);
	}

	SECTION("Duplicate flag in argument") {
		std::string flags = "Abd";
		op_args.push_back(flags + "ddd");

		REQUIRE_NOTHROW(
			itemlist.process_op(OP_EDITFLAGS, true, &op_args));
		REQUIRE(item->flags() == flags);
	}

	SECTION("Unauthorized values in arguments") {
		std::string flags = "Abd";
		SECTION("Numbers") {
			op_args.push_back(flags + "1236");

			REQUIRE_NOTHROW(itemlist.process_op(
					OP_EDITFLAGS, true, &op_args));
			REQUIRE(item->flags() == flags);
		}
		SECTION("Symbols") {
			op_args.push_back(flags +
				"%^\\*;\'\"&~#{([-|`_/@)]=}$£€µ,;:!?./§");

			REQUIRE_NOTHROW(itemlist.process_op(
					OP_EDITFLAGS, true, &op_args));
			REQUIRE(item->flags() == flags);
		}
		SECTION("Accents") {
			op_args.push_back(flags + "¨^");

			REQUIRE_NOTHROW(itemlist.process_op(
					OP_EDITFLAGS, true, &op_args));
			REQUIRE(item->flags() == flags);
		}
	}

	SECTION("All possible flags at once") {
		std::string flags =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		op_args.push_back(flags);

		REQUIRE_NOTHROW(
			itemlist.process_op(OP_EDITFLAGS, true, &op_args));
		REQUIRE(item->flags() == flags);
	}
}

TEST_CASE("OP_SAVE writes an article's attributes to the specified file",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile saveFile;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::vector<std::string> op_args;
	op_args.push_back(saveFile.get_path());

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_pubDate(test_pubDate);
	item->set_description(test_description);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);

	feed->add_item(item);
	itemlist.set_feed(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_SAVE, true, &op_args));

	TestHelpers::AssertArticleFileContent(saveFile.get_path(),
		test_title,
		test_author,
		test_pubDate_str,
		test_url,
		test_description);
}

TEST_CASE("OP_HELP command is processed", "[ItemListFormAction]")
{
	Controller c;
	RegexManager regman;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_regexmanager(&regman);
	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	feed->add_item(item);
	itemlist.set_feed(feed);

	v.push_itemlist(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_HELP));
}

TEST_CASE("OP_HARDQUIT command is processed", "[ItemListFormAction]")
{
	Controller c;
	RegexManager regman;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_regexmanager(&regman);
	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	v.push_itemlist(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_HARDQUIT));
}

TEST_CASE("Navigate back and forth using OP_NEXT and OP_PREVIOUS",
	"[ItemListFormAction]")
{
	// We are using the OP_SHOWURLS command to print the current
	// article'attibutes to a file, and assert the position was indeed
	// updated.
	Controller c;
	TestHelpers::TempFile articleFile;
	RegexManager regman;
	newsboat::View v(&c);
	ConfigContainer cfg;
	cfg.set_configvalue(
		"external-url-viewer", "tee > " + articleFile.get_path());
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;
	std::string line;

	std::string first_article_title = "First_Article";
	std::string second_article_title = "Second_Article";
	std::string prefix_title = "Title: ";

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_regexmanager(&regman);
	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_title(first_article_title);
	feed->add_item(item);

	std::shared_ptr<RssItem> item2 = std::make_shared<RssItem>(&rsscache);
	item2->set_title(second_article_title);
	feed->add_item(item2);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);

	v.push_itemlist(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_NEXT));
	itemlist.process_op(OP_SHOWURLS);

	std::ifstream fileStream(articleFile.get_path());
	std::getline(fileStream, line);
	REQUIRE(line == prefix_title + second_article_title);

	REQUIRE_NOTHROW(itemlist.process_op(OP_PREV));
	itemlist.process_op(OP_SHOWURLS);

	fileStream.seekg(0);
	std::getline(fileStream, line);
	REQUIRE(line == prefix_title + first_article_title);
}

TEST_CASE("OP_TOGGLESHOWREAD switches the value of show-read-articles",
	"[ItemListFormAction]")
{
	Controller c;
	RegexManager regman;
	newsboat::View v(&c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_regexmanager(&regman);
	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	feed->add_item(item);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);
	itemlist.set_feed(feed);
	v.push_itemlist(feed);

	SECTION("True to False") {
		v.get_cfg()->set_configvalue("show-read-articles", "yes");
		REQUIRE_NOTHROW(itemlist.process_op(OP_TOGGLESHOWREAD));
		REQUIRE_FALSE(v.get_cfg()->get_configvalue_as_bool(
				"show-read-articles"));
	}
	SECTION("False to True") {
		v.get_cfg()->set_configvalue("show-read-articles", "no");
		REQUIRE_NOTHROW(itemlist.process_op(OP_TOGGLESHOWREAD));
		REQUIRE(v.get_cfg()->get_configvalue_as_bool(
				"show-read-articles"));
	}
}

TEST_CASE("OP_PIPE_TO pipes an article's content to an external command",
	"[ItemListFormAction]")
{
	Controller c;
	newsboat::View v(&c);
	TestHelpers::TempFile articleFile;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FilterContainer filters;

	std::vector<std::string> op_args;
	op_args.push_back("tee > " + articleFile.get_path());

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_pubDate(test_pubDate);
	item->set_description(test_description);

	ItemListFormAction itemlist(&v, itemlist_str, &rsscache, &filters, &cfg);

	feed->add_item(item);
	itemlist.set_feed(feed);

	REQUIRE_NOTHROW(itemlist.process_op(OP_PIPE_TO, true, &op_args));

	TestHelpers::AssertArticleFileContent(articleFile.get_path(),
		test_title,
		test_author,
		test_pubDate_str,
		test_url,
		test_description);
}
