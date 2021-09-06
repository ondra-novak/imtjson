/*
 * jwt.cpp
 *
 *  Created on: 14. 12. 2019
 *      Author: ondra
 */

#include <string_view>
#include "jwt.h"

#include "base64.h"
#include "binary.h"
#include "object.h"


namespace json {


Value parseJWT(const std::string_view token, PJWTCrypto crypto,
							const AbstractJWTCrypto::SignMethod *forceMethod) {

	try {
		Split splt(token,".");
		std::string_view sheader = splt();
		std::string_view sbody = splt();
		std::string_view ssign = splt();
		std::string_view header_body(sheader.data(), (sbody.data() - sheader.data())+sbody.length());

		if (crypto != nullptr && forceMethod!=nullptr) {
			Binary sign = base64url->decodeBinaryValue(ssign).getBinary(base64url);
			if (!crypto->verify(header_body, *forceMethod, sign)) return undefined;
			crypto = nullptr;
		}
		Value body = Value::fromString(base64url->decodeBinaryValue(sbody).getString());
		if (crypto != nullptr) {
			Binary sign = base64url->decodeBinaryValue(ssign).getBinary(base64url);
			Value hdr = Value::fromString(base64url->decodeBinaryValue(sheader).getString());
			std::string_view alg = hdr["alg"].getString();
			std::string_view kid = hdr["kid"].getString();
			if (!crypto->verify(header_body, {alg, kid}, sign)) return undefined;
		}
		return body;
	} catch (...) {
		return undefined;
	}
}

Value parseJWTHdr(const std::string_view token) {
	try {
		Split splt ( token,".");
		std::string_view sheader = splt();
		return Value::fromString(base64url->decodeBinaryValue(sheader).getString());
	} catch (...) {
		return undefined;
	}
}

std::string serializeJWT(Value payload, PJWTCrypto crypto) {
	auto m = crypto->getPreferredMethod();
	return serializeJWT(payload, crypto, m);
}

std::string serializeJWT(Value payload, PJWTCrypto crypto, std::string_view kid) {
	auto m = crypto->getPreferredMethod(kid);
	return serializeJWT(payload, crypto, m);
}

std::string serializeJWT(Value payload, PJWTCrypto crypto, const AbstractJWTCrypto::SignMethod &method) {
	Value header = Object({
		{"alg", method.alg},
		{"kid", method.kid.empty()?Value():Value(method.kid)}});
	return serializeJWT(header, payload, crypto, method);
}

Value checkJWTTime(const Value value) {
	return checkJWTTime(value, std::chrono::system_clock::now());
}

Value checkJWTTime(const Value value, std::chrono::system_clock::time_point tp) {
	std::uint64_t tse = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
	Value exp = value["exp"];
	Value nbf = value["nbf"];
	if (exp.type() == json::number) {
		if (exp.getUIntLong() < tse) return json::undefined;
	}
	if (nbf.type() == json::number) {
		if (nbf.getUIntLong() > tse) return json::undefined;
	}
	return value;
}

std::string serializeJWT(Value header, Value payload, PJWTCrypto crypto,
							const AbstractJWTCrypto::SignMethod &method) {
	String h = header.stringify();
	String b = payload.stringify();
	std::string buffer;
	buffer.reserve(h.length()*4/3+b.length()*4/3+256);
	auto bufferFill = [&](std::string_view b) {buffer.append(b.data(),b.length());};
	base64url->encodeBinaryValue(map_str2bin(h.str()),bufferFill);
	buffer.push_back('.');
	base64url->encodeBinaryValue(map_str2bin(b.str()),bufferFill);
	Binary sign = crypto->sign(buffer, method);
	buffer.push_back('.');
	base64url->encodeBinaryValue(sign, bufferFill);

	//this need to remove extra space reserved for buffer
	return std::string(buffer.data(), buffer.length());
}



JWTCryptoContainer::JWTCryptoContainer(Map &&map, const SignMethod &preferred)
	:cmap(std::move(map)), preferred(preferred) {
	prepare();
}

JWTCryptoContainer::JWTCryptoContainer(const Map &map, const SignMethod &preferred)
	:cmap(map),preferred(preferred) {
	prepare();
}


JWTCryptoContainer::SignMethod JWTCryptoContainer::getPreferredMethod(std::string_view kid) const {
	if (kid.empty()) return preferred;
	auto end = cmap.end();
	auto iter = std::lower_bound(cmap.begin(),cmap.end(),Item{kid,nullptr}, isLess);
	if (iter == end || std::string_view((*iter).first) != kid) return preferred;
	return SignMethod {
		(*iter).second->getPreferredMethod().alg, kid
	};
}

Binary JWTCryptoContainer::sign(std::string_view message, SignMethod method) const {
	auto iter = std::lower_bound(cmap.begin(),cmap.end(),Item{method.kid,nullptr}, isLess);
	auto end = cmap.end();
	if (iter == end || std::string_view((*iter).first) != method.kid) return Binary();
	return (*iter).second->sign(message,method);

}

bool JWTCryptoContainer::verify(std::string_view message, SignMethod method, BinaryView signature) const {
	auto iter = std::lower_bound(cmap.begin(),cmap.end(),Item{method.kid,nullptr}, isLess);
	auto end = cmap.end();
	if (iter == end || std::string_view((*iter).first) != method.kid) return false;
	return (*iter).second->verify(message,method,signature);
}

const JWTCryptoContainer::Map& JWTCryptoContainer::getMap() const {
	return cmap;
}

bool JWTCryptoContainer::isLess(const Item &a, const Item &b) {
	if (a.first < b.first) return true;
	if (a.first > b.first) return false;
	if (a.second == b.second) return false;
	if (a.second == nullptr) return true;
	if (b.second == nullptr) return false;
	return (a.second->getPreferredMethod().alg < b.second->getPreferredMethod().alg);
}

void JWTCryptoContainer::prepare() {
	std::sort(cmap.begin(), cmap.end(), isLess);
}

}

json::JWTTokenCache::JWTTokenCache(unsigned int limit):limit(limit) {
	cache1.reserve(limit);
	cache2.reserve(limit);
}

json::Value json::JWTTokenCache::get(const std::string_view &token) const {
	auto iter = cache1.find(token);
	if (iter == cache1.end()) {
		iter = cache2.find(token);
		if (iter == cache1.end()) {
			return Value();
		}
	}
	return iter->second;
}

void json::JWTTokenCache::store(const std::string_view &token, json::Value content) {
	Value kt (token,content);
	cache1.emplace(kt.getKey(), kt);
	if (cache1.size() >= limit) {
		std::swap(cache1, cache2);
		cache1.clear();
	}
}

void json::JWTTokenCache::clear() {
	cache1.clear();
	cache2.clear();
}

std::size_t json::JWTTokenCache::Hash::operator ()(const std::string_view &token) const {
	static Base64Table t(Base64Table::base64urlchars);
	constexpr unsigned int hlen = (sizeof(std::size_t) * 4 / 3);
	std::string_view tail = token.substr(token.size()- hlen);
	unsigned char buff[hlen+2];
	Base64Encoding::decoderCore(buff, tail, tail.size(), t);
	return *reinterpret_cast<std::size_t *>(buff);
}
