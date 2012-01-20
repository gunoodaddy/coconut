#include <string>
#include <iostream>
#if defined(WIN32)
#include <conio.h>
#endif

#define NDEBUG
#define JSON_SAFE
#include <libjson/libjson.h>

void ParseJSON(const JSONNode & n){
	JSONNode::const_iterator i = n.begin();
	while (i != n.end()){
		// recursively call ourselves to dig deeper into the tree
		if (i -> type() == JSON_ARRAY || i -> type() == JSON_NODE){
			ParseJSON(*i);
		}

		// get the node name and value as a string
		std::string node_name = i -> name();

		printf("> %s => %s\n", node_name.c_str(), i -> as_string().c_str());
	
		//increment the iterator
		++i;
	}
}

static int errorCounter = 0;

void errorCallback(void *) {
    ++errorCounter;
	printf(">> ERROR!\n");
}

bool Callback(JSONNode & test, size_t remainpos, void * ide) {
	std::string jc = test.write();
	printf(">> SUCC! : %d\n%s\n", remainpos, jc.c_str());
	ParseJSON(test);
/*
	JSONNode &a = test.at("RootA");
	printf(">> %s\n", a.as_string().c_str());
	JSONNode &aa = test.at("ChildNode");
	JSONNode &aaa = aa.at("ChildA");
	printf(">> %s\n", aaa.as_string().c_str());
	*/

	return false;
}

int main(void) {
	/*
	JSONNode n(JSON_NODE);
	n.push_back(JSONNode("String Node", "String Value"));
	n.push_back(JSONNode("Integer Node", 42));
	n.push_back(JSONNode("Floating Point Node", 3.14));
	n.push_back(JSONNode("Boolean Node", true));
	std::string jc = n.write_formatted();
	std::cout << jc << std::endl;

	//JSONNode parse = libjson::parse(jc);
	JSONNode parse = libjson::parse(json);
	printf("PARSE OK\n");
	
	//ParseJSON(parse);
*/
	std::string json_arr = "{123123{\"RootA\":[1,2,3,4,5,6,7,8,9]}";
	std::string json = "{\"RootA\":\"Value in parent node\",\"ChildNode\":{\"ChildA\":\"String Value\",\"ChildB\":\"42\"}}";
	std::string json2 = "{\"RootA\":\"Value in parent node\",\"ChildNode\":{\"ChildA\":\"String Value\",\"ChildB\":\"42\"}}{\"RootA\":\"Value in parent node\",\"ChildNode\":{\"ChildA\":\"String Value\",\"ChildB\":\"42\"}}";

	std::string json3 = "[ { \"house_number\" : 42, \"road\" : \"East Street\", \"town\" : \"Newtown\", \"county\" : \"Essex\", \"country\" : \"England\" }, { \"house_number\" : 1, \"road\" : \"West Street\", \"town\" : \"Hull\", \"county\" : \"Yorkshire\", \"country\" : \"England\" }, { \"house_number\" : 12, \"road\" : \"South Road\", \"town\" : \"Aberystwyth\", \"county\" : \"Dyfed\", \"country\" : \"Wales\" }, { \"house_number\" : 45, \"road\" : \"North Road\", \"town\" : \"Paignton\", \"county\" : \"Devon\", \"country\" : \"England\" }, { \"house_number\" : 78, \"road\" : \"Upper Street\", \"town\" : \"Ware\", \"county\" : \"Hertfordshire\", \"country\" : \"England\" } ]";

	//JSONNode resnode = libjson::parse(json);
	//ParseJSON(resnode);

	JSONStream test(Callback, errorCallback, (void*)0xDEADBEEF);
	//test << JSON_TEXT("{}[]");

	size_t ntot = 0;
	std::string sbuf;
	std::string current;
	current = json;
	current = json2;
	current = json3;
	current = json_arr;
	while(ntot < current.size()) {
		char buffer[19] = {0, };
		size_t size = sizeof(buffer) - 1;
		if(size > current.size() - ntot) {
			size = current.size() - ntot;
		}

		strncpy(buffer, current.c_str() + ntot, size);

		sbuf.append(buffer);
		printf("BUFFER : %s\n", sbuf.c_str());

		ntot += size;
		test << JSON_TEXT(buffer);
	}

	//JSONStream test2(Callback, errorCallback, (void*)0xDEADBEEF);
	//test2 << sbuf;

	printf("OK\n");
#if defined(WIN32)
	_getch();
#endif
	return 0;
}

