#ifndef NEWLINE_PIPE_H
#define NEWLINE_PIPE_H
#include <deque>

template <typename T>
class newline_pipe 
{
	public:
		newline_pipe()
		{
		};
		~newline_pipe()
		{
		};
		void write(T src)
		{
			q.push_back(src);
		};
		void write(T * src)
		{
			unsigned int i = 0;
			while(src[i] != 0x00)
				q.push_back(src[i++]);
		};
		void write(T * src, unsigned int max)
		{
			unsigned int i = 0;
			while(i < max)
				q.push_back(src[i++]);
		};
		unsigned int read(T * dst, unsigned int n)
		{
			unsigned int i = 0;
			unsigned int bytes = (n > q.size() ? q.size() : n);
			while(i < bytes)
				{ dst[i++] = q.front(); q.pop_front(); }
			return i;

		};
		unsigned int peek(T * dst, unsigned int n)
		{
			unsigned int i = 0;
			unsigned int bytes = (n > q.size() ? q.size() : n);
			while(i < bytes)
				{ dst[i] = q[i]; i++;}
			return i;
		}
		unsigned int readLine(T * dst, unsigned int max)
		{
			unsigned int i;
			i = canReadLine();
			if(i && i <= max)
			{
				i = read(dst, i);
				dst[i] = '\0';
				return i;
			}
			else
				return 0;

		};
		unsigned int canReadLine(void)
		{
			unsigned int i = 0;
			while(i < q.size())
			{
				if(q[i] == '\n')	
					break;
				//else if(q[i] == '\r')
					//break;
				i++;
			}
			if(i == q.size())
				return 0;
			else
				return i + 1;
		};
		unsigned int canReadHTTPLine(void)
		{
			unsigned int i = 3;
			if(q.size() < 4)
				return 0;
			while(i < q.size())
			{
				if(q[i - 3] == '\r' &&
				   q[i - 2] == '\n' &&
				   q[i - 1] == '\r' &&
				   q[i] == '\n')	
					break;
				i++;
			}
			if(i == q.size())
				return 0;
			else
				return i + 1;
		};
		unsigned int canRead(void)
		{
			return q.size();
		};
	private:
		std::deque <T> q;
		
};
#endif //NEWLINE_PIPE_H
