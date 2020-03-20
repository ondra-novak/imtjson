/*
 * jwt.h
 *
 *  Created on: 14. 12. 2019
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_JWT_H_
#define SRC_IMTJSON_JWT_H_
#include <chrono>
#include <unordered_map>

#include "../../../shared/stringview.h"
#include "value.h"

using ondra_shared::StrViewA;

namespace json {

class AbstractJWTCrypto: public RefCntObj {
public:

	AbstractJWTCrypto() {}
	virtual ~AbstractJWTCrypto() {}

	struct SignMethod {
		///algoritm
		StrViewA alg;
		///Key ID (optional, can be empty)
		StrViewA kid;
	};

	///Returns preferred signature method
	/** In most cases, the implementation can prefer one method to sign token. If the
	 * method is not specified, the preferred method is used
	 * @param kid required KeyID (optional)
	 * @return preferred method. If there is no preferred method, returns empty string
	 */
	virtual SignMethod getPreferredMethod(StrViewA kid = StrViewA()) const = 0;
	///Creates signature
	/**
	 * @param message message to sign
	 * @param method method, mandatory. If not known, use getPreferredMethod to obtain
	 * supported method
	 * @return binary object contains signature. If method is not supported, returns empty object
	 */
	virtual Binary sign(StrViewA message, SignMethod method) const = 0;
	///Verifies signature
	/**
	 * @param message message which signature to verify
	 * @param method method used to sign this message
	 * @param signature signature
	 * @retval true signature is valid
	 * @retval false signature is not valid or method is not supported
	 */
	virtual bool verify(StrViewA message, SignMethod method, BinaryView signature) const = 0;

	AbstractJWTCrypto(const AbstractJWTCrypto &) = delete;
	AbstractJWTCrypto &operator=(const AbstractJWTCrypto &) = delete;
};

using PJWTCrypto = RefCntPtr<AbstractJWTCrypto>;

///Contains multiple sign methods
/** This container can be useful if you need multiple sign methods or multiple keys
 * Each item is identified by SignMethods. One of the method is preferred.
 *
 * You need to prepare a vector of all the keys before the object is constructed. You
 * cannot modify already constructed object. However you can retrieve current map, modify
 * the map and create new instance from the new map;
 */
class JWTCryptoContainer: public AbstractJWTCrypto {
public:

	using Item = std::pair<std::string, PJWTCrypto>;
	using Map = std::vector<Item>;

	JWTCryptoContainer(Map &&map, const SignMethod &preferred);
	JWTCryptoContainer(const Map &map, const SignMethod &preferred);

	virtual SignMethod getPreferredMethod(StrViewA kid = StrViewA()) const override;;
	virtual Binary sign(StrViewA message, SignMethod method) const override;
	virtual bool verify(StrViewA message, SignMethod method, BinaryView signature) const override;

	const Map &getMap() const;

protected:
	std::vector<Item> cmap;
	SignMethod preferred;

	static bool isLess(const Item &a, const Item &b);
	void prepare();

};

///Parse JWT token
/**
 *
 * @param token token to parse
 * @param crypto object contains implementation of cryptografy to verify token. This field
 * can be nullptr to skip verifying the signature.
 * @param forceMethod ignores header and forces cryptographic method to verify token.
 * @return returns parsed token as JSON object. If the token is not valid, signature is
 * not valid or there is another error, return is undefined
 *
 * @note if the crypto is nullptr, signature is not checked. Note that if the token's
 * cryptographic method is "none", it is still passed through the PJWTCrypto object
 * to check, whether this kind of token is accepted
 */
Value parseJWT(const StrViewA token, PJWTCrypto crypto = nullptr, const AbstractJWTCrypto::SignMethod *forceMethod = nullptr);

///Parses JWT header for more info if needed (otherwise, header is not returned by parseJWT)
Value parseJWTHdr(const StrViewA token);

///Checks whether the token's body is valid for current  time
/**
 * @param value token's body. Must have fields exp and nbf. If nbf is not defined, then
 * token is valid from the beginning of the epoch. If the exp is not defined, then
 * token is valid till end of epoch
 * @return
 */
Value checkJWTTime(const Value value);

Value checkJWTTime(const Value value, std::chrono::system_clock::time_point tp);

///Serializes and signs token.
/**
 * @param payload object to serialize
 * @param crypto crypto object to sign the token. Preferred method will be used
 * @return JWT token
 */
std::string serializeJWT(Value payload, PJWTCrypto crypto);


///Serializes and signs token
/**
 * @param payload object to serialize
 * @param crypto crypto object
 * @param kid keyID
 * @return signed token
 */
std::string serializeJWT(Value payload, PJWTCrypto crypto, StrViewA kid);

///Serializes and signs token using selected method
/**
 * @param payload object to serialize
 * @param crypto crypto object
 * @param method
 * @return serialized JWT token
 */
std::string serializeJWT(Value payload, PJWTCrypto crypto, const AbstractJWTCrypto::SignMethod &method);


///Serializes and signs token using selected method
/**
 *
 * @param header header. Note this function doesn't modify the header. You must supply correct header
 * @param payload payload
 * @param crypto crypto object
 * @param method method to use
 * @return serialized object
 */
std::string serializeJWT(Value header, Value payload, PJWTCrypto crypto, const AbstractJWTCrypto::SignMethod &method);

///Simple cache for JWT tokens to speed up parsing and validating
/**
 * It is intended to store parsed and valid tokens. The cache is limited to centrain number of tokens.
 *
 * Because the parsing and validating token involves hashing and cryptographic functions, the parsing token is slow operation. Because tokens
 * are used repeatedly during their validity, it can make validating faster to include a cache into process.
 *
 * The cache is implemented using unordered map (hash map) with very fast hashing function. The function uses fact, that the signature
 * is already kind of hash, so hash is not really done, the hash map simple uses the tail of the token as hash.
 */
class JWTTokenCache {
public:

	///Initialize cache and specify its limit size
	/**
	 *
	 * @param limit specifies limit for the cache in items. However the cache is split into two halfs. The limit specifies maximum items in
	 * the each half. Once both halfs are filled, one of them is cleared.
	 *
	 * Example, specifiying number 1000 causes, that there can be 1000-2000 tokens in the cache. Once the count reaches 2000, the half of tokens
	 * are removed from the cache
	 */
	JWTTokenCache(unsigned int limit);
	///Find and get token from the cache
	/**
	 * @param token token
	 * @return returns parsed token, or undefined when token is new
	 */
	json::Value get(const StrViewA &token) const;
	///stores token to the cache
	/**
	 * @param token token string
	 * @param content parsed content
	 */
	void store(const StrViewA &token, json::Value content);
	///Clear the cache (whole)
	void clear();


protected:
	struct Hash {
		std::size_t operator()(const StrViewA &token) const;
	};
	using CacheMap=std::unordered_map<StrViewA, json::Value, Hash>;

	CacheMap cache1, cache2;
	unsigned int limit;

};


}
#endif
