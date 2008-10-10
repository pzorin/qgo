#include <QRegExp>
#include "msghandler.h"
/* 
 * extratcs and returns the element between 2 delimiters, at a given delimiter count
 * taken from old IGS parser code.
 */
QString MsgHandler::element(const QString &line, int index, const QString &del1, const QString &del2, bool killblanks)
{
	int len = line.length();
	int idx = index;
	int i;
	QString sub;

	// kill trailing white spaces
	while (/*(int)*/ line[len-1] < 33 && len > 0)
		len--;

	// right delimiter given?
	if (del2.isEmpty())
	{
		// ... no right delimiter
		// element("a b c", 0, " ") -> "a"
		// element("  a b c", 0, " ") -> "a"  spaces at the beginning are skipped
		// element("a b c", 1, " ") -> "b"
		// element(" a  b  c", 1, " ") -> "b", though two spaces between "a" and "b"

		// skip (delimiters) spaces at the beginning
		i = 0;
//		while (i < len && line[i] == del1)
		while (i < len && line[i] == ' ')
			i++;

		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == ' ' && i < len-1 && line[i+1] == ' ')
				i++;

			// look for delimiters, maybe more in series
			if (line.mid(i,del1.length()) == del1)
				idx--;
			else if (idx == 0)
				sub += line[i];
		}
	}
	else
	{
		// ... with right delimiter
		// element("a b c", 0, " ", " ") -> "b"
		// element("(a) (b c)", 0, "(", ")") -> "a"
		// element("(a) (b c)", 1, "(", ")") -> "b c"
		// element("(a) (b c)", 0, " ", ")") -> "(b c"
		// element("(a) (b c)", 1, " ", ")") -> "c"
		// element("(a) (b c)", 1, "(", "EOL") -> "b c)"

		// skip spaces at the beginning
		i = 0;
		while (i < len && line[i] == ' ')
			i++;

		// seek left delimiter
		idx++;
	
		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == ' ' && i < len-1 && line[i+1] == ' ')
				i++;

			if ((idx != 0 && line.mid(i,del1.length()) == del1) ||
						  (idx == 0 && line.mid(i,del2.length()) == del2))
			{
				idx--;
			}
			else if (idx == 0)
			{
				// EOL (End Of Line)?
				if (del2 == QString("EOL"))
				{
					// copy until end of line
					for (int j = i; j < len; j++)
						if (!killblanks || line[j] != ' ')
							sub += line[j];

					// leave loop
					idx--;
				}
				else if (!killblanks || line[i] != ' ')
					sub += line[i];
			}
		}
	}
	
	return sub;
}


unsigned int MsgHandler::idleTimeToSeconds(QString time)
{
	QString i = time;
	/* I guess its either minutes or seconds here, not both */
	if(time.contains("m"))
	{
		i.replace(QRegExp("[m*]"), "");
		return (60 * i.toInt());
	}
	else
	{
		i.replace(QRegExp("s"), "");
		return i.toInt();
	}
	/*m1 = time1; m2 = time2;
	s1 = time1; s2 = time2;
	m1.replace(QRegExp("[m*]"), "");
	m2.replace(QRegExp("[m*]"), "");
	s1.replace(QRegExp("[*m]"), "");
	s1.replace(QRegExp("s"), "");
	s2.replace(QRegExp("[*m]"), "");
	s2.replace(QRegExp("s"), "");
	int seconds1 = (m1.toInt() * 60) + s1.toInt();
	int seconds2 = (m2.toInt() * 60) + s2.toInt();*/
}

void MsgHandler::fixRankString(QString * rank)
{
	if(rank->at(rank->length() - 1) == '*')
		rank->truncate(rank->length() - 1);
	else if(*rank == "NR") {}
	else
			*rank += "?";
}
