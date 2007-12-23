/*
 * GStreamer
 * Copyright 2007 Edgard Lima <edgard.lima@indt.org.br>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "metadataiptc.h"
#include "metadataparseutil.h"
#include "metadatatags.h"

GST_DEBUG_CATEGORY (gst_metadata_iptc_debug);
#define GST_CAT_DEFAULT gst_metadata_iptc_debug

#ifndef HAVE_IPTC

void
metadataparse_iptc_tag_list_add (GstTagList * taglist, GstTagMergeMode mode,
    GstAdapter * adapter, MetadataTagMapping mapping)
{

  if (mapping & METADATA_TAG_MAP_WHOLECHUNK) {
    GST_LOG
        ("IPTC not defined, here I should send just one tag as whole chunk");
    metadataparse_util_tag_list_add_chunk (taglist, mode, GST_TAG_IPTC,
        adapter);
  }

}


void
metadatamux_iptc_create_chunk_from_tag_list (guint8 ** buf, guint32 * size,
    const GstTagList * taglist)
{
  /* do nothing */
}

#else /* ifndef HAVE_IPTC */

#include <iptc-data.h>
#include <iptc-tag.h>
#include <string.h>
#include <gst/gsttaglist.h>

typedef struct _tag_MEUserData
{
  GstTagList *taglist;
  GstTagMergeMode mode;
} MEUserData;

typedef struct _tag_MapIntStr
{
  IptcRecord record;
  IptcTag iptc;
  const gchar *str;
} MapIntStr;

static void
iptc_data_foreach_dataset_func (IptcDataSet * dataset, void *user_data);

/* *INDENT-OFF* */
static MapIntStr mappedTags[] = {
  {IPTC_RECORD_APP_2, IPTC_TAG_OBJECT_NAME,      /*ASCII*/ GST_TAG_TITLE       /*STRING*/},
  {IPTC_RECORD_APP_2, IPTC_TAG_BYLINE,           /*ASCII*/ GST_TAG_COMPOSER    /*STRING*/},
  {IPTC_RECORD_APP_2, IPTC_TAG_CAPTION,          /*ASCII*/ GST_TAG_DESCRIPTION /*STRING*/},
  {IPTC_RECORD_APP_2, IPTC_TAG_COPYRIGHT_NOTICE, /*ASCII*/ GST_TAG_COPYRIGHT   /*STRING*/},
  {0, 0, NULL}
};
/* *INDENT-ON* */

static const gchar *
metadataparse_iptc_get_tag_from_iptc (IptcTag iptc, GType * type,
    IptcRecord * record)
{
  int i = 0;

  while (mappedTags[i].iptc) {
    if (iptc == mappedTags[i].iptc) {
      *type = gst_tag_get_type (mappedTags[i].str);
      *record = mappedTags[i].record;
      break;
    }
    ++i;
  }

  return mappedTags[i].str;

}

static IptcTag
metadataparse_iptc_get_iptc_from_tag (const gchar * tag, GType * type,
    IptcRecord * record)
{
  int i = 0;

  while (mappedTags[i].iptc) {
    if (0 == strcmp (mappedTags[i].str, tag)) {
      *type = gst_tag_get_type (tag);
      *record = mappedTags[i].record;
      break;
    }
    ++i;
  }

  return mappedTags[i].iptc;

}

void
metadataparse_iptc_tag_list_add (GstTagList * taglist, GstTagMergeMode mode,
    GstAdapter * adapter, MetadataTagMapping mapping)
{
  const guint8 *buf;
  guint32 size;
  IptcData *iptc = NULL;
  MEUserData user_data = { taglist, mode };

  if (adapter == NULL || (size = gst_adapter_available (adapter)) == 0) {
    goto done;
  }

  /* add chunk tag */
  if (mapping & METADATA_TAG_MAP_WHOLECHUNK)
    metadataparse_util_tag_list_add_chunk (taglist, mode, GST_TAG_IPTC,
        adapter);

  if (!(mapping & METADATA_TAG_MAP_INDIVIDUALS))
    goto done;

  buf = gst_adapter_peek (adapter, size);

  iptc = iptc_data_new_from_data (buf, size);
  if (iptc == NULL) {
    goto done;
  }

  iptc_data_foreach_dataset (iptc, iptc_data_foreach_dataset_func,
      (void *) &user_data);

done:

  if (iptc)
    iptc_data_unref (iptc);

  return;

}

static void
iptc_data_foreach_dataset_func (IptcDataSet * dataset, void *user_data)
{

  char buf[1024];
  MEUserData *meudata = (MEUserData *) user_data;
  GType type;
  IptcRecord record;
  const gchar *tag =
      metadataparse_iptc_get_tag_from_iptc (dataset->tag, &type, &record);
  const gchar *value = iptc_dataset_get_as_str (dataset, buf, 1024);

  if (!tag)
    goto done;

  gst_tag_list_add (meudata->taglist, meudata->mode, tag, value, NULL);

done:

  GST_LOG ("name -> %s", iptc_tag_get_name (dataset->record, dataset->tag));
  GST_LOG ("title -> %s", iptc_tag_get_title (dataset->record, dataset->tag));
  GST_LOG ("description -> %s", iptc_tag_get_description (dataset->record,
          dataset->tag));
  GST_LOG ("value = %s", value);
  GST_LOG ("record = %d", dataset->record);

  return;

}


static void
metadataiptc_for_each_tag_in_list (const GstTagList * list, const gchar * tag,
    gpointer user_data)
{
  IptcData *iptc = (IptcData *) user_data;
  IptcTag iptc_tag;
  IptcRecord record;
  GType type;
  IptcDataSet *dataset = NULL;
  gboolean new_dataset = FALSE;
  gchar *tag_value = NULL;

  iptc_tag = metadataparse_iptc_get_iptc_from_tag (tag, &type, &record);

  if (!iptc_tag)
    goto done;

  dataset = iptc_data_get_dataset (iptc, record, iptc_tag);

  if (!dataset) {
    dataset = iptc_dataset_new ();
    new_dataset = TRUE;
  }

  iptc_dataset_set_tag (dataset, record, iptc_tag);

  if (gst_tag_list_get_string (list, tag, &tag_value)) {
    iptc_dataset_set_data (dataset, tag_value, strlen (tag_value),
        IPTC_DONT_VALIDATE);
    g_free (tag_value);
    tag_value = NULL;
  }


  if (new_dataset)
    iptc_data_add_dataset (iptc, dataset);

done:

  if (dataset)
    iptc_dataset_unref (dataset);
}

void
metadatamux_iptc_create_chunk_from_tag_list (guint8 ** buf, guint32 * size,
    const GstTagList * taglist)
{
  IptcData *iptc = NULL;
  GstBuffer *iptc_chunk = NULL;
  const GValue *val = NULL;

  if (!(buf && size))
    goto done;
  if (*buf) {
    g_free (*buf);
    *buf = NULL;
  }
  *size = 0;

  val = gst_tag_list_get_value_index (taglist, GST_TAG_IPTC, 0);
  if (val) {
    iptc_chunk = gst_value_get_buffer (val);
    if (iptc_chunk) {
      iptc = iptc_data_new_from_data (GST_BUFFER_DATA (iptc_chunk),
          GST_BUFFER_SIZE (iptc_chunk));
    }
  }

  if (!iptc) {
    iptc = iptc_data_new ();
  }

  gst_tag_list_foreach (taglist, metadataiptc_for_each_tag_in_list, iptc);

  iptc_data_save (iptc, buf, size);


done:

  if (iptc)
    iptc_data_unref (iptc);

  return;
}

#endif /* else (ifndef HAVE_IPTC) */