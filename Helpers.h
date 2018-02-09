#ifndef _HELPERSH_
#define _HELPERSH_
#include <sstream>
#include <string>
#include <vector>

using namespace std;
class Helpers
{
    public:
        static vector<string> split(string s, char delim)
        {
            stringstream temp;
            vector<string> elems(0);
            if (s.size() == 0 || delim == 0)
                return elems;
           for (unsigned int i=0; i<s.length(); ++i) 
            {
                char c=s[i];
                if(c == delim)
                {
                    elems.push_back(temp.str());
                    temp.clear();
                }
                else
                    temp << c;
            }
            if (temp.str().size() > 0)
                elems.push_back(temp.str());
                return elems;
            }

        //Splits string s with a list of delimiters in delims (it's just a list, like if we wanted to
        //split at the following letters, a, b, c we would make delims="abc".
        static vector<string> split(string s, string delims)
        {
            stringstream temp;
            vector<string> elems(0);
            bool found;
            if(s.size() == 0 || delims.size() == 0)
                return elems;
             for (unsigned int i=0; i < s.length(); ++i) 
            {
                char c=s[i];
                char next=' ';
                if(i+1<s.length())
                    next=s[i+1];
                found = false;

            for (unsigned int j=0; j<delims.length(); ++j) 
            {
                char d=delims[j];
                    if(next!=' '){
                       if(c==d){
                             bool nextFound=false;
                             for (int k=0; k<delims.length(); ++k) 
            {
                char d2=delims[k];
                        if(d2==next)
                            nextFound=true;
                        } 
                        if(nextFound)
                            {
                             elems.push_back(temp.str());
                        temp.clear();
                        found = true;
                        break;


                            }
                    }
                    }
                    else if (c == d)
                    {
                        elems.push_back(temp.str());
                        temp.clear();
                        found = true;
                        break;
                    }
                    
                }
                if(!found)
                    temp << c;
            }
            if(temp.str().size() > 0)
                elems.push_back(temp.str());
            return elems;
        }
};
#endif
