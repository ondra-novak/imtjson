///@note this header is mostly included into CPP file, because contains implementation as well
#ifndef SRC_IMTJSON_JWTCRYPTO_H_
#define SRC_IMTJSON_JWTCRYPTO_H_

#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <memory>

#include "binary.h"
#include "jwt.h"
#include "shared/stringview.h"

using ondra_shared::BinaryView;
using ondra_shared::StrViewA;


namespace json {

namespace alg {

class SignBuffer {
public:
	unsigned char buff[1024];
	unsigned int len = 1024;
	Binary getBinary() const {return Value(BinaryView(buff,len),base64url).getBinary(base64url);}
	bool check(BinaryView b) const {
		return b == BinaryView(buff,len);
	}
};

inline bool verifyRS256(BinaryView message, BinaryView signature, RSA *rsa) {
	unsigned char buffer[SHA256_DIGEST_LENGTH];
	SHA256(message.data, message.length,buffer);
	return RSA_verify(NID_sha256, buffer, SHA256_DIGEST_LENGTH, signature.data, signature.length, rsa) == 1;
}
inline bool verifyRS384(BinaryView message, BinaryView signature, RSA *rsa) {
	unsigned char buffer[SHA384_DIGEST_LENGTH];
	SHA384(message.data, message.length,buffer);
	return RSA_verify(NID_sha384, buffer, SHA384_DIGEST_LENGTH, signature.data, signature.length, rsa) == 1;
}
inline bool verifyRS512(BinaryView message, BinaryView signature, RSA *rsa) {
	unsigned char buffer[SHA512_DIGEST_LENGTH];
	SHA512(message.data, message.length,buffer);
	return RSA_verify(NID_sha512, buffer, SHA512_DIGEST_LENGTH, signature.data, signature.length, rsa) == 1;
}
inline Binary signRS256(BinaryView message, RSA *rsa) {
	unsigned char buffer[SHA256_DIGEST_LENGTH];
	SignBuffer sbuff;
	SHA256(message.data, message.length,buffer);
	RSA_sign(NID_sha256, buffer, SHA256_DIGEST_LENGTH, sbuff.buff, &sbuff.len, rsa);
	return sbuff.getBinary();
}
inline Binary signRS384(BinaryView message, RSA *rsa) {
	unsigned char buffer[SHA384_DIGEST_LENGTH];
	SignBuffer sbuff;
	SHA384(message.data, message.length,buffer);
	RSA_sign(NID_sha384, buffer, SHA384_DIGEST_LENGTH, sbuff.buff, &sbuff.len,rsa);
	return sbuff.getBinary();
}
inline Binary signRS512(BinaryView message, RSA *rsa) {
	unsigned char buffer[SHA512_DIGEST_LENGTH];
	SHA512(message.data, message.length,buffer);
	SignBuffer sbuff;
	RSA_sign(NID_sha512, buffer, SHA512_DIGEST_LENGTH, sbuff.buff, &sbuff.len, rsa);
	return sbuff.getBinary();
}
inline Binary signHS256(BinaryView message, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha256(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.getBinary();
}
inline Binary signHS384(BinaryView message, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha384(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.getBinary();
}
inline Binary signHS512(BinaryView message, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha512(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.getBinary();
}
inline bool verifyHS256(BinaryView message, BinaryView sign, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha256(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.check(sign);
}
inline bool verifyHS384(BinaryView message, BinaryView sign, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha384(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.check(sign);
}
inline bool verifyHS512(BinaryView message, BinaryView sign, BinaryView key) {
	SignBuffer sbuff;
	HMAC(EVP_sha512(),key.data, key.length, message.data, message.length,sbuff.buff, &sbuff.len);
	return sbuff.check(sign);
}

struct PECDSA_SIG_free {void operator()(ECDSA_SIG *p)const{ECDSA_SIG_free(p);}};
using PECDSA_SIG = std::unique_ptr<ECDSA_SIG, PECDSA_SIG_free>;

inline void storeESSign(PECDSA_SIG &&sig, SignBuffer &sbuff) {
	const BIGNUM *r;
	const BIGNUM *s;
	ECDSA_SIG_get0(sig.get(),&r,&s);
	unsigned char *c = sbuff.buff;
	c = c + BN_bn2bin(r, c);
	c = c + BN_bn2bin(s, c);
	sbuff.len = c - sbuff.buff;
}

inline auto parseESSign(BinaryView signature) {
	BinaryView br(signature.substr(0, signature.length/2));
	BinaryView bs(signature.substr(signature.length/2));
	BIGNUM *r = BN_new(), *s = BN_new();
	BN_bin2bn(br.data, br.length, r);
	BN_bin2bn(bs.data, bs.length, s);
	PECDSA_SIG sig (ECDSA_SIG_new());
	ECDSA_SIG_set0(sig.get(),r,s);
	return sig;
}

inline bool verifyES256(BinaryView message, BinaryView signature, EC_KEY *eck) {
	unsigned char buffer[SHA256_DIGEST_LENGTH];
	auto sig = parseESSign(signature);
	SHA256(message.data, message.length,buffer);
	return ECDSA_do_verify(buffer, SHA256_DIGEST_LENGTH, sig.get(), eck) == 1;
}
inline bool verifyES384(BinaryView message, BinaryView signature, EC_KEY *eck) {
	unsigned char buffer[SHA384_DIGEST_LENGTH];
	auto sig = parseESSign(signature);
	SHA384(message.data, message.length,buffer);
	return ECDSA_do_verify(buffer, SHA384_DIGEST_LENGTH, sig.get(), eck) == 1;
}
inline bool verifyES512(BinaryView message, BinaryView signature, EC_KEY *eck) {
	unsigned char buffer[SHA512_DIGEST_LENGTH];
	auto sig = parseESSign(signature);
	SHA512(message.data, message.length,buffer);
	return ECDSA_do_verify(buffer, SHA512_DIGEST_LENGTH, sig.get(),  eck) == 1;
}
inline Binary signES256(BinaryView message, EC_KEY *eck) {
	unsigned char buffer[SHA256_DIGEST_LENGTH];
	SignBuffer sbuff;
	SHA256(message.data, message.length,buffer);
	storeESSign(PECDSA_SIG(ECDSA_do_sign(buffer, SHA256_DIGEST_LENGTH, eck)), sbuff);
	return sbuff.getBinary();
}
inline Binary signES384(BinaryView message, EC_KEY *eck) {
	unsigned char buffer[SHA384_DIGEST_LENGTH];
	SignBuffer sbuff;
	SHA384(message.data, message.length,buffer);
	storeESSign(PECDSA_SIG(ECDSA_do_sign(buffer, SHA384_DIGEST_LENGTH, eck)), sbuff);
	return sbuff.getBinary();
}


inline Binary signES512(BinaryView message, EC_KEY *eck) {
	unsigned char buffer[SHA512_DIGEST_LENGTH];
	SHA512(message.data, message.length,buffer);
	SignBuffer sbuff;
	storeESSign(PECDSA_SIG(ECDSA_do_sign(buffer, SHA512_DIGEST_LENGTH, eck)), sbuff);
	return sbuff.getBinary();
}
}


class JWTCrypto_RS: public AbstractJWTCrypto {
public:
	JWTCrypto_RS(RSA *rsa, int pref_size=256):rsa(rsa),size(pref_size) {}

	virtual SignMethod getPreferredMethod(StrViewA kid = StrViewA()) const {
		switch (size) {
		default:
		case 256: return {"RS256",kid};
		case 384: return {"RS384",kid};
		case 512: return {"RS512",kid};
		}
	}

	virtual Binary sign(StrViewA message, SignMethod method) const {
		if (method.alg == "RS256") return alg::signRS256(BinaryView(message), rsa);
		if (method.alg == "RS384") return alg::signRS384(BinaryView(message), rsa);
		if (method.alg == "RS512") return alg::signRS512(BinaryView(message), rsa);
		return Binary();
	}

	virtual bool verify(StrViewA message, SignMethod method, BinaryView signature) const {
		if (method.alg == "RS256") return alg::verifyRS256(BinaryView(message), signature, rsa);
		if (method.alg == "RS384") return alg::verifyRS384(BinaryView(message), signature, rsa);
		if (method.alg == "RS512") return alg::verifyRS512(BinaryView(message), signature, rsa);
		return false;
	}

	~JWTCrypto_RS() {
		RSA_free(rsa);
	}
protected:
	RSA *rsa;
	int size;
};

class JWTCrypto_HS: public AbstractJWTCrypto {
public:
	JWTCrypto_HS(std::string key, int pref_size=256):key(std::move(key)),size(pref_size) {}

	virtual SignMethod getPreferredMethod(StrViewA kid = StrViewA()) const {
		switch (size) {
		default:
		case 256: return {"HS256",kid};
		case 384: return {"HS384",kid};
		case 512: return {"HS512",kid};
		}
	}

	virtual Binary sign(StrViewA message, SignMethod method) const {
		BinaryView b((StrViewA(key)));
		if (method.alg == "HS256") return alg::signHS256(BinaryView(message), b);
		if (method.alg == "HS384") return alg::signHS384(BinaryView(message), b);
		if (method.alg == "HS512") return alg::signHS512(BinaryView(message), b);
		return Binary();
	}

	virtual bool verify(StrViewA message, SignMethod method, BinaryView signature) const {
		BinaryView b((StrViewA(key)));
		if (method.alg == "HS256") return alg::verifyHS256(BinaryView(message), signature, b);
		if (method.alg == "HS384") return alg::verifyHS384(BinaryView(message), signature, b);
		if (method.alg == "HS512") return alg::verifyHS512(BinaryView(message), signature, b);
		return false;
	}

protected:
	std::string key;
	int size;
};


class JWTCrypto_ES: public AbstractJWTCrypto {
public:

	///Initialize JWTCrypto
	/**
	 * @param key EC_KEY initialized with either public or private key (depend on whether you need to sign or verify)
	 * @param size size of key to choose correct type of curve. Valid values are 256,384,512
	 */
	JWTCrypto_ES(EC_KEY *key, int size):key(key),size(size) {}

	virtual SignMethod getPreferredMethod(StrViewA kid = StrViewA()) const {
		switch (size) {
		default:
		case 256: return {"ES256",kid};
		case 384: return {"ES384",kid};
		case 512: return {"ES512",kid};
		}
	}

	virtual Binary sign(StrViewA message, SignMethod method) const {
		if (method.alg == "ES256" && size == 256) return alg::signES256(BinaryView(message), key);
		if (method.alg == "ES384" && size == 384) return alg::signES384(BinaryView(message), key);
		if (method.alg == "ES512" && size == 512) return alg::signES512(BinaryView(message), key);
		return Binary();
	}

	virtual bool verify(StrViewA message, SignMethod method, BinaryView signature) const {
		if (method.alg == "ES256" && size == 256) return alg::verifyES256(BinaryView(message), signature, key);
		if (method.alg == "ES384" && size == 384) return alg::verifyES384(BinaryView(message), signature, key);
		if (method.alg == "ES512" && size == 512) return alg::verifyES512(BinaryView(message), signature, key);
		return false;
	}

	~JWTCrypto_ES() {
		EC_KEY_free(key);
	}

	///inicializes key
	/**
	 * @param size size of key (256,384,512)
	 * @return generated empty key
	 */
	static EC_KEY *initKey(int size) {
		switch (size) {
		case 256: return EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
		case 384: return EC_KEY_new_by_curve_name(NID_secp384r1);
		case 512: return EC_KEY_new_by_curve_name(NID_secp521r1);
		default: return nullptr;
		}
	}


	///Loads public key from the BASE64 string
	/**
	 * @param size size of key (256,384,512)
	 * @param keyBase64 base64
	 * @return
	 */
	static EC_KEY *importPublicKey(int size, const StrViewA &keyBase64) {


		Value k = base64url->decodeBinaryValue(keyBase64);
		Binary b = k.getBinary(base64url);

		EC_KEY *key = initKey(size);

		const unsigned char *data = b.data;
		EC_KEY *nwkey = o2i_ECPublicKey(&key,&data, b.length);
		if (nwkey == nullptr) {
			EC_KEY_free(key);
			return nullptr;
		}
		if (nwkey != key) {
			EC_KEY_free(key);
			key = nwkey;
		}
		return key;
	}


	static EC_KEY *importPrivateKey(int size, const BinaryView &b) {
		std::unique_ptr<BIGNUM,decltype(&BN_free)> priv_key (BN_bin2bn(b.data, b.length, BN_new()), &BN_free);
		std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> key(initKey(size), &EC_KEY_free);
	    std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx (BN_CTX_new(), &BN_CTX_free);
	    if (ctx == nullptr) return nullptr;

	    const EC_GROUP *group = EC_KEY_get0_group(key.get());

	    std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)> pub_key(EC_POINT_new(group), &EC_POINT_free);
	    if (pub_key == nullptr) return nullptr;

	    if (!EC_POINT_mul(group, pub_key.get(), priv_key.get(), NULL, NULL, ctx.get())) return nullptr;

	    EC_KEY_set_private_key(key.get(), priv_key.release());
	    EC_KEY_set_public_key(key.get(), pub_key.release());

	    return key.release();
	}

	static EC_KEY *importPrivateKey(int size, const StrViewA &keyBase64)
	{
		Value k = base64url->decodeBinaryValue(keyBase64);
		Binary b = k.getBinary(base64url);
		if (b.empty()) return nullptr;

		return importPrivateKey(size, b);

	}

	static std::string exportPublicKey(EC_KEY *key) {
		EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
		int nSize = i2o_ECPublicKey(key, NULL);
		std::vector<unsigned char> binpubkey(nSize);
		unsigned char *pbinpk = binpubkey.data();
		if (i2o_ECPublicKey(key, &pbinpk) != nSize) return std::string();
		std::string ret;
		ret.reserve(nSize * 4/3+10);
		base64url->encodeBinaryValue(BinaryView(binpubkey.data(), binpubkey.size()),
				[&](StrViewA data) {ret.append(data.data, data.length);});
		return ret;
	}
	static std::string exportPublicKeyPEM(EC_KEY *key) {
		BIO *b = BIO_new(BIO_s_mem());
		PEM_write_bio_EC_PUBKEY(b,key);
		BUF_MEM *mem;
		BIO_get_mem_ptr(b, &mem);
		std::string ret;
		ret.append(mem->data, mem->length);
		BIO_free(b);
		return ret;
	}
	static std::string exportPrivKeyPEM(EC_KEY *key) {
		BIO *b = BIO_new(BIO_s_mem());
		PEM_write_bio_ECPrivateKey(b,key,0,0,0,0,0);
		BUF_MEM *mem;
		BIO_get_mem_ptr(b, &mem);
		std::string ret;
		ret.append(mem->data, mem->length);
		BIO_free(b);
		return ret;
	}


	std::string exportPublicKey() const {
		return exportPublicKey(key);
	}

protected:
	EC_KEY *key;
	int size;

};

}






#endif /* SRC_IMTJSON_JWTCRYPTO_H_ */
