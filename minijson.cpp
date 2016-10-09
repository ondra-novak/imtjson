// minijson.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "src/immujson/stringref.h"
#include "src/immujson/refcnt.h"
#include "src/immujson/ivalue.h"
#include "src/immujson/parser.h"
#include "src/immujson/serializer.h"
#include "src/immujson/edit.h"



int main()
{
	unsigned int xx = 10;
	float z = 2.4f;
	json::Value v = json::Value::fromString("{\"kazma\":10,\"doodle\":[0.003899,2,3,true,null]}");
	json::Value a({ 10,23.4,"ahoj",u8"čeština","kkk20",false,-132.324e-14,xx,z });
	v = json::Object(v)("harcha", a);

	{
		json::Object edit(v);
		edit.object("prvni").object("druhy").array("treti").add({ 10,30,1234.134578 });
		{
			json::Object2Object lev2 = edit.object("prvni").object("druhy");
			lev2.set("paty", { "test",true,json::Object("kkk", 32.1)("lll", false) });
		}
		v = edit;
	}


	std::string x = v.toString();
	OutputDebugStringA(x.c_str());
    return 0;
}

