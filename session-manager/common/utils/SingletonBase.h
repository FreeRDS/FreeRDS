/**
 * Singleton pattern template
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thinstuff.at>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SINGLETON_BASE_HEADER_H
#define __SINGLETON_BASE_HEADER_H

/**
* @file	SingletonBase.h
*
* @brief	Singleton implementation using c++ templates.
*
* Singleton pattern:
*
* The singleton design pattern is a way of enforcing only one instance of an object. 
* This is achieved by making 3 fairly simple steps to a class. 
* Firstly making the constructor private, creating a static member variable that will contain the instance
* and then creating a static method for accessing the instance.
*
*
* @author	Martin Haimberger
* @date		4/22/2009
*/

#include <stdio.h>

/**
* @def	SINGLETON_ADD_INITIALISATION(className)
* 
* @brief	Adds protected constructor, destructor and copyconstructor for the class \<classNam\>. It also adds
*			the class <code>SingletonBase</code> as friendclass. This is necesary if using the singleton pattern with
*           templates, otherwise the singleton can not be constructed the first time.
*
*/

#define SINGLETON_ADD_INITIALISATION(className) friend class SingletonBase<className>; \
												protected:\
													className();\
													~className();\
													className( const className& );
/**
* @class	SingletonBase
*
* @brief	Singleton implementation using c++ templates.
*
* Singleton pattern:
*
* The singleton design pattern is a way of enforcing only one instance of an object.
* This is achieved by making 3 fairly simple steps to a class.
* Firstly making the constructor private, creating a static member variable that will contain the instance
* and then creating a static method for accessing the instance.
*
*
* To use this singleton template you have to do following steps:
* - Implment singleton using the template (ConnectionSingleton)
* - Use singleton
*
* Implment singleton using the template:
*
* @code
#define CONNECTION_SINGLETON ConnectionSingleton::instance()

class ConnectionSingleton : public SingletonBase<ConnectionSingleton> {

SINGLETON_ADD_INITIALISATION(ConnectionSingleton)

};
* @endcode
*
* Use singleton:
*
* @code
ConnectionSingleton::instance().foobar()

or

CONNECTION_SINGLETON.foobar()
* @endcode
*
* @author	Martin Haimberger
* @date		4/22/2009
*/
template <class Derived> class SingletonBase
{
public:

	/**
	* @fn	static Derived& instance()
	*
	* @brief	Getter of the Singelton. Returns the static instance of the class.
	*
	* @retval	Derived - One and only static instance of the class.
	*/

	static Derived& instance();

protected:

	/**
	* @fn	SingletonBase()
	*
	* @brief	Default protected constructor.
	*
	* The protected constructor prevents, that another instance is generated, unless the static one.
	* The constructor has to be protected instead of private, to allow applying it as template.
	*/

	SingletonBase() {
	};

	/**
	* @fn	~SingletonBase()
	*
	* @brief	Finaliser.
	*
	* @author	Martin Haimberger
	* @date		4/22/2009
	*/

	~SingletonBase(){};

private:

	/**
	* @fn	SingletonBase( const SingletonBase& )
	*
	* @brief	private copy constructor.
	*
	* The copy constructor is in private scope, to prevent creation of another instance via copy constructor.
	*/

	SingletonBase( const SingletonBase& );
};


template <class Derived>
Derived& SingletonBase<Derived>::instance()
{
	static Derived _instance;
	return _instance;
}

#endif
