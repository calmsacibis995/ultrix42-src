/* m_seq.c - print out a message sequence */

#include "../h/mh.h"
#include <stdio.h>

/* new version from VJ 2/90 - faster? */

char *
m_seq(mp, cp)
	struct msgs *mp;
	char *cp;
{
	int mask;
	register int i, j;
	register char *bp;
	static char buffer[BUFSIZ*2];	/* for big sequences */

	if (strcmp(current, cp) == 0) {
		if (mp->curmsg) {
			(void) sprintf(buffer, "%s", m_name(mp->curmsg));
			return (buffer);
		} else
			return (NULL);
	}
	for (i = 0; mp->msgattrs[i]; i++)
		if (strcmp(mp->msgattrs[i], cp) == 0)
			break;
	
	if (! mp->msgattrs[i])
		return (NULL);

	mask = EXISTS | (1 << (FFATTRSLOT + i));
	bp = buffer;
	for (i = mp->lowmsg; i <= mp->hghmsg; ++i) {
		if ((mp->msgstats[i] & mask) != mask)
			continue;

		if (bp > buffer)
			*bp++ = ' ';

		(void) sprintf(bp, "%s", m_name(i));
		bp += strlen(bp);
		j = i;
		for (++i; i <= mp->hghmsg && (mp->msgstats[i] & mask) == mask;
		     ++i)
			;
		if (i - j > 1) {
			(void) sprintf(bp, "-%s", m_name(i - 1));
			bp += strlen(bp);
		}
	}
	return (bp > buffer? buffer : NULL);
}
