/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


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
