#ifndef STRING_H_
#define STRING_H_
#ifdef __cplusplus
class String {
	public:
		String() {
			//create with 16 elements by default
			buffer = new char[16];
		}
		String(String str) {
			int len = str.length();
			buffer = new char[len+1];
			buffer = strcpy(buffer, str);
		}
		String(const char * str) {
			buffer = new char[strlen(buffer) + 1];
			buffer = strcpy(buffer, str);
		}
		~String() {
			if(buffer != null) {
				delete [] buffer;
			}
		}

	public:
		const char * getString() { return buffer; }

		String operator+(const String& lh, const String & rh) {
			return new String(strcat(lh, rh));
		}
	private:
		char * buffer;
};

char * operator+(const char * lh, const char * rh) {
	return strcat(lh, rh);
}

#endif


#endif /* STRING_H_ */
