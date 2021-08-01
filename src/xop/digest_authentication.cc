#include "digest_authentication.h"
#include "md5.hpp"

DigestAuthentication::DigestAuthentication(std::string realm, std::string username, std::string password)
	: m_realm(realm)
	, m_username(username)
	, m_password(password)
{

}

std::string DigestAuthentication::get_realm() const
{ 
	return m_realm; 
}

std::string DigestAuthentication::get_username() const
{ 
	return m_username; 
}

std::string DigestAuthentication::get_password() const
{ 
	return m_password; 
}

std::string DigestAuthentication::get_nonce()
{
	return md5::generate_nonce();
}

std::string DigestAuthentication::get_response(std::string nonce, std::string cmd, std::string url)
{
	//md5(md5(<username>:<realm> : <password>) :<nonce> : md5(<cmd>:<url>))

	auto hex1 = md5::md5_hash_hex(m_username + ":" + m_realm + ":" + m_password);
	auto hex2 = md5::md5_hash_hex(cmd + ":" + url);
	auto response = md5::md5_hash_hex(hex1 + ":" + nonce + ":" + hex2);

	return response;
}

