#pragma once


namespace json {


	///Function object capable to remove comments from the stream
	/** @see removeComments */
	template<typename Fn>
	class CommentedJSON {
	public:

		CommentedJSON(const Fn &fn) : fn(fn), nextChar(-1), inStr(false) {}

		int operator()() const {
			//if we have charged a next char
			if (nextChar != -1) {				
				int d = nextChar;
				//reset next char
				nextChar = -1;
				//return the stored char
				return d;
			}
			else {
				//load next char
				int c = fn();
				//if we are in string 
				if (inStr) {
					//escape character charges next character
					if (c == '\\') {
						nextChar = fn();
					}
					//quotes ends string
					else if (c == '"') {
						inStr = false;
					}
					//always return current character
					return c;
				}
				//we are not in string and quotes found
				else if (c == '"') {
					//set inStr
					inStr = true;
					//return it
					return c;
				}
				//we are not in string and character '/' loaded for '//' or '/*'
				else if (c == '/') {
					//read next character
					int d = fn();
					//is it '/'?
					if (d == '/') {
						//skip everything until end of line
						while (d != '\r' && d != '\n' && d != -1) {
							d = fn();
						}
						//fetch next character
						return operator()();
					}
					//is it '*'?
					else if (d == '*') {
						//store previous char here
						int prev = 0;
						//load next char
						d = fn();
						//skip until end of file or '*/' found
						while (d != -1 && (d != '/' || prev != '*')) {
							//remember prev character
							prev = d;
							//load next one
							d = fn();
						}
						//fetch next character
						return operator()();
					}
					else {
						//all tests fails, 
						//charge next char
						nextChar = d;
						//return current						
						return c;
					}
				}
				else {
					//no match? pass through
					return c;
				}
			}
		}



	protected:

		Fn fn;
		mutable int nextChar;
		mutable bool inStr;

	};


	///Function converts input stream to stream where comments are removed
	/** The comments are formated similar way as C/C++ comments using double
	 slash or slash and dot (asi line comment or block comment). Comments 
	 are supported only outside ot strings. If these chars appears inside of the
	 string, they are left untouched there. However it is allowed to put 
	 comment inside a number, array, object and also for tokens are transparent,
	 so 'tr/ *comment* /ue' will be parsed correctly as true (there is unwanted
	 space between / and * to prevent confusion of C++ compiler. The space is not
	 part of sequence)
	 
	 
	 @param fn function which is source of the stream
	 @return function which contains stream where comments are removed
	 */

	template<typename Fn>
	CommentedJSON<Fn> removeComments(const Fn &fn) {
		return CommentedJSON<Fn>(fn);
	}

}