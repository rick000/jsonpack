# jsonpack
make serialize c++ type to json or parse json to c++ easier
# jsonpack

- make serialize c++ type to json or parse json to c++ easier
- only one header file 
- requirements
	- c++17 
	- nlohmann:json (easy to adapter  to  other json lib)

# example
## normal struct type
- we have json string like bellow
```
{"a": 123, "b": "asdffd"}
```
- first we define a struct 
```
struct ExampleStruct1 
{
	int a;
	string b;
};
```
- define how to serialize/deserialize json
```
REGISTER_JSONPACK_TYPE(ExampleStruct1,
	REGISTER_JSONPACK_FIELD(a, "a"),
	REGISTER_JSONPACK_FIELD(b, "b")
);
```
- serialize:
```
	ExampleStruct1 est { 123, "hello world!" };
	nlohmann::json json_data = {};
	jsonpack::type_to_json(est, json_data);
	
	// for array
	std::vector<ExampleStruct1> ests;
	// do some initialize
	jsonpack::type_to_json(ests, json_data);
```
- deserialize:
```
	auto json_string = "{\"a\": 123, \"b\": \"asdffd\"}";
	auto json_data = nlohmann::json::parse(json_string, nullptr, false);
	ExampleStruct1 st { 111, "" }; 
	jsonpack::json_to_type(json_data, st); // also support std::vector
```

## nested type
```
struct ExampleStruct2 
{
	int st2;
	ExampleStruct1 st;
};
```
- define how to serialize/deserialize json
```
REGISTER_JSONPACK_TYPE(ExampleStruct2,
	REGISTER_JSONPACK_FIELD(st2, "jsonname"),
	REGISTER_JSONPACK_FIELD(st, "jsonname for st")
);
```
call ```jsonpack::json_to_type``` or ```jsonpack::type_to_json``` to do the work!

## more complicated type
```
struct ExampleStruct3
{
	int a;
	string b;
	std::vector<int> c;
	std::vector<ExampleStruct2> d;
};
REGISTER_JSONPACK_TYPE(ExampleStruct3,
	REGISTER_JSONPACK_FIELD(a, "a"),
	REGISTER_JSONPACK_FIELD(b, "jsonb"),
	REGISTER_JSONPACK_FIELD(c, "c"),
	REGISTER_JSONPACK_FIELD(d, "d")
);
```
```jsonpack::json_to_type``` or ```jsonpack::type_to_json``` still works!
