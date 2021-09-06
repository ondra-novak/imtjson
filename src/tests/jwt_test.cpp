/*
 * jwt_test.cpp
 *
 *  Created on: 14. 12. 2019
 *      Author: ondra
 */

#include "../imtjson/jwt.h"
#include "../imtjson/jwtcrypto.h"
#include "../imtjson/object.h"
#include "testClass.h"

using namespace json;


void runJwtTests(TestSimple &tst) {

	tst.test("jwt.create-check.ES256","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {

		EC_KEY *key = JWTCrypto_ES::importPrivateKey(256,"TestPrivateKey");
		PJWTCrypto jwtc = new JWTCrypto_ES(key,256);
		std::string token = serializeJWT(Object({{"aaa",123},{"bbb",323}}),jwtc);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.create-check.ES384","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {

		EC_KEY *key = JWTCrypto_ES::importPrivateKey(384,"TestPrivateKey");
		PJWTCrypto jwtc = new JWTCrypto_ES(key,384);
		std::string token = serializeJWT(Object({{"aaa",123},{"bbb",323}}),jwtc);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.create-check.ES512","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {

		EC_KEY *key = JWTCrypto_ES::importPrivateKey(512,"TestPrivateKey");
		PJWTCrypto jwtc = new JWTCrypto_ES(key,512);
		std::string token = serializeJWT(Object({{"aaa",123},{"bbb",323}}),jwtc);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};

	tst.test("jwt.create - public.ES512","AwD6GulvdFhrFOu5ma_pKRu1SDMaIiCT0BgjSYHkokwgE1fldMsL05NJn8l2JKbXn_XGl1cEaMMQe4jb0U10IaaLNA") >> [](std::ostream &out) {
		EC_KEY *key = JWTCrypto_ES::importPrivateKey(512,"TestPrivateKey");
		std::string pubKey = JWTCrypto_ES::exportPublicKey(key);
		JWTCrypto_ES dummy(key,512);
		out << pubKey;
	};
	tst.test("jwt.check.ES512","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {
		std::string token = "eyJhbGciOiJFUzUxMiJ9.eyJhYWEiOjEyMywiYmJiIjozMjN9.AOsvHDwRI5scssdZ4XU4inTk_GTlLwmIvd4AD2AlZKruwN_X0MMmG6C8fMM6MlwPS9aCZS0VM8N0CPrD1bSn6gatAK3QXK663C5KqOW1vnkr-c-1QMxz5fiJu2xtpvIHKW42z-Kb5iiTPZF5p_5ugq0-yvQ7QPgFYtN-JaNri1Sm-xU3";
		EC_KEY *key = JWTCrypto_ES::importPublicKey(512,"AwD6GulvdFhrFOu5ma_pKRu1SDMaIiCT0BgjSYHkokwgE1fldMsL05NJn8l2JKbXn_XGl1cEaMMQe4jb0U10IaaLNA");
		PJWTCrypto jwtc = new JWTCrypto_ES(key,512);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.check.ES512-failed","\"undefined\"") >> [](std::ostream &out) {
		std::string token = "eyJhbGciOiJFUzUxMiJ9.eyJhYWEiOjEyMywiYmJjIjozMjN9.MIGHAkFld_cQbTJZdJncEZMdVDDegaqYnwDMVfOCMfoez8hRA4zhcIa47ojgSFYSupfCLb1TP1slngJOdJHkGWEP4poQFAJCATWAa9PbHCR4JGA5Pf2l4MRcTzL4cLs4-_E8SCq6PFB9Lnoeip3g1sqPmyuYIMr5jHDoIDFb60O5Ntfn3m7Zqcxw";
		EC_KEY *key = JWTCrypto_ES::importPublicKey(512,"AwD6GulvdFhrFOu5ma_pKRu1SDMaIiCT0BgjSYHkokwgE1fldMsL05NJn8l2JKbXn_XGl1cEaMMQe4jb0U10IaaLNA");
		PJWTCrypto jwtc = new JWTCrypto_ES(key,512);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.create-check.HS256","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {

		PJWTCrypto jwtc = new JWTCrypto_HS("secret",256);
		std::string token = serializeJWT(Object({{"aaa",123},{"bbb",323}}),jwtc);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.check.HS256","{\"aaa\":123,\"bbb\":323}") >> [](std::ostream &out) {
		std::string token = "eyJhbGciOiJIUzI1NiJ9.eyJhYWEiOjEyMywiYmJiIjozMjN9.Wfb8UJS8zM3E_5OKVvwaMWNpj99cXvHBIf4wVxm3ndM";
		PJWTCrypto jwtc = new JWTCrypto_HS("secret",256);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};
	tst.test("jwt.check.HS256-failed","\"undefined\"") >> [](std::ostream &out) {
		std::string token = "eyJhbGciOiJIUzI1NiJ9.eyJhYWEiOjEyMywiYmJjIjozMjN9.Wfb8UJS8zM3E_5OKVvwaMWNpj99cXvHBIf4wVxm3ndM";
		PJWTCrypto jwtc = new JWTCrypto_HS("secret",256);
		Value v = parseJWT(token,jwtc);
		v.toStream(out);
	};



}


