/* editenv.c - tool to edit environment block.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009,2010,2013 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <grub/types.h>
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/util/install.h>
#include <grub/lib/envblk.h>
#include <grub/i18n.h>
#include <grub/emu/hostfile.h>

#include <errno.h>
#include <string.h>
#if !defined(_WIN32)
#include <libgen.h>
#endif

#define GRUB_ENVBLK_MESSAGE	"# WARNING: Do not edit this file by tools other than "PACKAGE"-editenv!!!\n"

void
grub_util_create_envblk_file (const char *name)
{
  FILE *fp;
  char *buf, *pbuf, *namenew;

#if !defined(_WIN32)
  ssize_t size = 1;
  char *rename_target = xstrdup (name);
  int rc;
#endif

  buf = xmalloc (DEFAULT_ENVBLK_SIZE);

  namenew = xasprintf ("%s.new", name);
  fp = grub_util_fopen (namenew, "wb");
  if (! fp)
    grub_util_error (_("cannot open `%s': %s"), namenew,
		     strerror (errno));

  pbuf = buf;
  memcpy (pbuf, GRUB_ENVBLK_SIGNATURE, sizeof (GRUB_ENVBLK_SIGNATURE) - 1);
  pbuf += sizeof (GRUB_ENVBLK_SIGNATURE) - 1;
  memcpy (pbuf, GRUB_ENVBLK_MESSAGE, sizeof (GRUB_ENVBLK_MESSAGE) - 1);
  pbuf += sizeof (GRUB_ENVBLK_MESSAGE) - 1;
  memset (pbuf , '#',
          DEFAULT_ENVBLK_SIZE - sizeof (GRUB_ENVBLK_SIGNATURE) - sizeof (GRUB_ENVBLK_MESSAGE) + 2);

  if (fwrite (buf, 1, DEFAULT_ENVBLK_SIZE, fp) != DEFAULT_ENVBLK_SIZE)
    grub_util_error (_("cannot write to `%s': %s"), namenew,
		     strerror (errno));


  if (grub_util_file_sync (fp) < 0)
    grub_util_error (_("cannot sync `%s': %s"), namenew, strerror (errno));
  free (buf);
  fclose (fp);

#if defined(_WIN32)
  if (grub_util_rename (namenew, name) < 0)
    grub_util_error (_("cannot rename the file %s to %s"), namenew, name);
#else
  while (1)
    {
      char *linkbuf;
      ssize_t retsize;

      linkbuf = xmalloc (size + 1);
      retsize = grub_util_readlink (rename_target, linkbuf, size);
      if (retsize < 0 && (errno == ENOENT || errno == EINVAL))
        {
          free (linkbuf);
          break;
        }
      else if (retsize < 0)
        {
          free (linkbuf);
          grub_util_error (_("cannot rename the file %s to %s: %m"), namenew, name);
        }
      else if (retsize == size)
        {
          free (linkbuf);
          size += 128;
          continue;
        }

      linkbuf[retsize] = '\0';
      if (linkbuf[0] == '/')
        {
          free (rename_target);
          rename_target = linkbuf;
        }
      else
        {
          char *dbuf = xstrdup (rename_target);
          const char *dir = dirname (dbuf);

          free (rename_target);
          rename_target = xasprintf ("%s/%s", dir, linkbuf);
          free (dbuf);
          free (linkbuf);
        }
    }

  rc = grub_util_rename (namenew, rename_target);
  if (rc < 0 && errno == EXDEV)
    {
      rc = grub_install_copy_file (namenew, rename_target, 1);
      grub_util_unlink (namenew);
    }

  free (rename_target);

  if (rc < 0)
    grub_util_error (_("cannot rename the file %s to %s: %m"), namenew, name);
#endif

  free (namenew);
}