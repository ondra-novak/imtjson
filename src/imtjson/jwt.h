/*
 * jwt.h
 *
 *  Created on: 14. 12. 2019
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_JWT_H_
#define SRC_IMTJSON_JWT_H_
#include <chrono>

#include "value.h"

namespace json {

class AbstractJWTCrypto: public RefCntObj {
public:

	AbstractJWTCrypto() {}

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


}
#endif
