#ifndef KRISVERS_D3D9TO12_HELLO_WORLD_HPP
#define KRISVERS_D3D9TO12_HELLO_WORLD_HPP

#include <exception>
#include <iostream>
#include <cstring>

#ifdef WIN32
	#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
	#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef __FUNCTION_NAME__
	#ifdef WIN32
		#define __FUNCTION_NAME__ __FUNCTION__
	#else
		#define __FUNCTION_NAME__ __func__
	#endif
#endif

#define assert_msg_action(v, msg, action) if (!(v)) { std::cerr << "Assertion failed (" << __FUNCTION_NAME__ << "() in " << __FILENAME__ << ":" << __LINE__ << "):\n    " << #v << "\n\"" << msg << "\"\n"; std::cerr.flush(); action; throw std::exception(msg); }
#define assert_msg(v, msg) assert_msg_action(v, msg,)

#endif