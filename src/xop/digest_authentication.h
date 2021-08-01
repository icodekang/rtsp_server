#ifndef _DIGEST_AUTHENTICATION_H_
#define _DIGEST_AUTHENTICATION_H_

#include <cstdint>
#include <string>
#include "vlog.h"

class DigestAuthentication
{
public:
	DigestAuthentication(std::string realm, std::string username, std::string password);

	std::string get_realm() const;
	std::string get_username() const;
	std::string get_password() const;
	std::string get_nonce();
	std::string get_response(std::string nonce, std::string cmd, std::string url);

	virtual ~DigestAuthentication() = default;

private:
	std::string m_realm;
	std::string m_username;
	std::string m_password;

};

#endif // _DIGEST_AUTHENTICATION_H_
