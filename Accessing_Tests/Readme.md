/************************************************************************/
/*   
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
/************************************************************************/


/*
Compiler / Software : AVR/GNU C++ Compiler Toolchain / Atmel Studio 7.0
Targeted device : Atmega328P 

Author : bebenlebricolo
date: 02 December 2017 (02/12/2017)

Accessing_deep_class_method_test1.cpp
// Trying to access low level classes different ways.
// My purpose is to try many different ways of accessing internal data of 
// sub-levels classes (most simple ones / low-levels)

On the following example :  A is the most "high-level" class of A, B and C classes. A contains 1 B object
							B is the middle one (used as dedicated container for sub-classes C). B contains 2 C objects
							C is the lowest level one. Contains only basic types (uint16_t)

Class A (has class B( has class C)))

// An usual way would be to access methods of C via methods of higher layers (B and A ones)
// For the most simple cases, like accessing a single value (using a getter), it could be annoying to 
// re-write the same methods for higher layers.
// -> (i.e. this only implies connecting high layer method to the same one of the right underneath layer)
//	A.meth(args) -> B.meth(args) -> C.meth(args) (copying 3 times the same method)

If thought about using pointers to get direct access to those low-levels classes.
If it's not THE way to go, it might be useful sometimes and allow you not to re-write and connect all identical methods.
A.directC_Access()->c_methods(args)
*/
