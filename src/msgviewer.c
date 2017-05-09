//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "msgviewer.h"
#include "stations.h"
#include "utils.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Message data interpreters.
//------------------------------------------------------------------------------

static
bool mdi_prn_idt(void *fli, void *msg_ptr)
{
   struct t_message     *msg     = (struct t_message*)msg_ptr;
   struct t_message_fli *field   = (struct t_message_fli*)fli;
   struct t_response    *resp;
   t_cl_error           err;
   register uint16      len;

   assert(field   != NULL);
   assert(msg     != NULL);

   if (field->field_md->out_spec == 'n') return(true);

   fprintf(msg->out_point->fs, "%s:", field->field_md->descr);

   len = strlen_utf8(field->field_md->descr);
   if (len < msg->mmd->msg_out_max_dl)
      fprint_sym(msg->out_point->fs, ' ', msg->mmd->msg_out_max_dl - len);
   fprintf(msg->out_point->fs, " ");

   if (field->field_md->out_spec == 'x')
   {
      if ((field->real_size * 2) > MAX_PRN_LEN)
      {
         fprintf(msg->out_point->fs, "\n");
         fprint_by_spec(msg->out_point->fs, field->field_data,
                       field->real_size, 'p', DUMP_WLIMIT);
      }
      else
         fprint_by_spec(msg->out_point->fs, field->field_data,
                       field->real_size, 'x', DUMP_WLIMIT);
   }
   else if (field->field_md->out_spec == 'm')
   {
      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, 'd', DUMP_WLIMIT);
      fprintf(msg->out_point->fs, " ");
      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, 't', DUMP_WLIMIT);
   }
   else
   {
//      if ((field->field_md->out_spec == 'b') && ((len * 8) > MAX_PRN_LEN))
//         fputc('\n', msg->out_point->fs);

      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, field->field_md->out_spec, DUMP_WLIMIT);
   }

   if (field->responses)
   {
      resp = cl_cur_to_first(field->responses, &err);
      if (resp)
      {
         fprintf(msg->out_point->fs, _(" ("));
         while (resp && (err == clec_noerr))
         {
            if (resp->error_level != 0)
               fprintf(msg->out_point->fs, _("%s [err = %d]"), resp->descr, resp->error_level);
            else
               fprintf(msg->out_point->fs, _("%s"), resp->descr, resp->error_level);
            resp = cl_next(field->responses, &err);
            if (resp) fprintf(msg->out_point->fs, ", ");
         }
         fprintf(msg->out_point->fs, _(")"));
      }

   }
  
   fprintf(msg->out_point->fs, "\n");
   return(true);
}
//------------------------------------------------------------------------------
static
int mdi_prn(struct t_message *msg)
// Message data interpreter for human-readable output.
{
   assert(msg != NULL);

   cl_iterate(msg->fields, &mdi_prn_idt, msg, NULL);

   if (msg->remainder_len)
   {
      fprintf(msg->out_point->fs, _("%s (%d bytes):\n"),
            (msg->remainder_descr) ?
            msg->remainder_descr : _("Remainder of message data"), msg->remainder_len);
      fprint_hexdump(msg->out_point->fs, msg->pointer, msg->remainder_len, DUMP_WLIMIT);
      fprintf(msg->out_point->fs, "\n");
   }
//   fprintf(out_point->fs, "%s\n", LINE_D);
   return(0);
}
//------------------------------------------------------------------------------
static
bool mdi_prn_ihr(void *hf_ptr, void *vp_ptr)
{
   struct t_header_field   *hf   = (struct t_header_field*)hf_ptr;
   struct t_full_msgdata   *vp   = (struct t_full_msgdata*)vp_ptr;
   struct t_metafield      mf;

   assert(hf      != NULL);
   assert(vp      != NULL);
   assert(vp->msg != NULL);
   assert(vp->pmd != NULL);

   if (strceq(hf->name, "$sender_name") || strceq(hf->name, "$reciever_name")) return(true);

   mf.name = hf->name;

   if (!get_metafield_by_name(vp, &mf)) return(false);

   if ((hf->out_spec == 'n') || (!hf->descr)) return(true);
   fprintf(vp->msg->out_point->fs, "%s: ", hf->descr);
   fprint_sym(vp->msg->out_point->fs, ' ', vp->pmd->header_fields_md->max_desc_len - strlen_utf8(hf->descr));

   if (hf->out_spec == 'm')
   {
      fprint_by_spec(vp->msg->out_point->fs, (uint8*)&(mf.value.data),
                    mf.size, 'd', DUMP_WLIMIT);
      fprintf(vp->msg->out_point->fs, ", ");
      fprint_by_spec(vp->msg->out_point->fs, (uint8*)&(mf.value.data),
                    mf.size, 't', DUMP_WLIMIT);
   }
   else
   {
      fprint_by_spec(vp->msg->out_point->fs,
                     (mf.type == mft_data) ? (uint8*)&(mf.value.data) : mf.value.data_ptr,
                     mf.size, hf->out_spec, DUMP_WLIMIT);
   }

   if (strceq(hf->name, "$crc"))
   {
      if (mf.value.data == vp->msg->crc_real)
      {
         fprintf(vp->msg->out_point->fs, _(" [correct]"));
      }
      else
      {
         fprintf(vp->msg->out_point->fs, _(" [Incorrect! Calculated CRC: 0x%04X]"),
                 vp->msg->crc_real);
      }
   }
   else
   {
      mf.name = NULL;
      if (strceq(hf->name, "$sender"))
      {
         mf.name = "$sender_name";
      }
      else if (strceq(hf->name, "$reciever"))
      {
         mf.name = "$reciever_name";
      }

      if (mf.name)
      {
         if (!get_metafield_by_name(vp, &mf)) return(false);
         fprintf(vp->msg->out_point->fs, " (%s)", mf.value.data_ptr);
      }  
   }
   fprintf(vp->msg->out_point->fs, "\n");
   return(true);
}
//------------------------------------------------------------------------------
static
bool shm_prn(struct t_full_msgdata *fmd)
// Message structure interpreter for human-readable output.
{
   FILE  *fs;
   struct t_message           *msg        =  fmd->msg;
   struct t_message_metadata  *msg_md     = msg->mmd;
   struct t_message_raw       *raw_msg    = msg->raw_message;
   uint16                     raw_data_sz = message_data_len(raw_msg);

   assert(fmd        != NULL);
   assert(msg        != NULL);
   assert(fmd->pmd   != NULL);

   msg_md   = msg->mmd;
   raw_msg  = msg->raw_message;

   if (msg_md && msg_md->is_compound)
      fprintf(msg->out_point->fs, _("%s\nCOMPOUND MESSAGE\n%s\n"), LINE_E, LINE_E);

   fs = msg->out_point->fs;

   cl_iterate(fmd->pmd->header_fields_md->fields_md, &mdi_prn_ihr, fmd, NULL);

   if (raw_data_sz)
   {
      if (msg->mmd)
      {
         if (!msg->out_only_headers)
         {
            if (!msg->mmd->is_compound)
            {
               fprintf(fs, _("\n***** Message data *****\n\n"));
               mdi_prn(msg);
            }
            else
            {
               fprintf(fs, _("\n%s\n***** Simple messages *****\n%s\n\n"),
                       LINE_D, LINE_D);

            }
         }
            
      }
      else
      {
         fprint_hexdump(fs, (uint8*)&(raw_msg->data), raw_data_sz, DUMP_WLIMIT);
         fprintf(fs, _("\n"));
      }
   }
   else
   {
      fprintf(fs, _("Message has not data...\n%s\n"), LINE_D);
   }

   if (msg->mmd && !msg->mmd->is_compound) fprintf(fs, "\n%s\n", LINE_D);

   fflush(fs);
   return(true);
}
//------------------------------------------------------------------------------
static
bool mdi_csv_idt(void *fli, void *user)
{
   struct t_message     *msg     = (struct t_message*)user;
   struct t_message_fli *field   = (struct t_message_fli*)fli;

   assert(msg     != NULL);
   assert(field   != NULL);

   if (field->field_md->out_spec == 'n') return(true);
   if (field->field_md->out_spec == 'm')
   {
      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, 'd', DUMP_WLIMIT);
      fprintf(msg->out_point->fs, ";");
      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, 't', DUMP_WLIMIT);
   }
   else
   {
      fprint_by_spec(msg->out_point->fs, field->field_data,
                    field->real_size, field->field_md->out_spec, 0);
   }
   fprintf(msg->out_point->fs, ";");

   return(true);
}
//------------------------------------------------------------------------------
static
bool mdi_csv_ihr(void *hf_ptr, void *vp_ptr)
{
   struct t_full_msgdata   *fmd  = (struct t_full_msgdata*)vp_ptr;
   struct t_header_field   *hf   = (struct t_header_field*)hf_ptr;
   struct t_metafield      mf;

   assert(hf      != NULL);
   assert(fmd     != NULL);

   mf.name = hf->name;
   if (!get_metafield_by_name(fmd, &mf)) return(false);

   if ((hf->out_spec == 'n') || (!hf->descr)) return(true);
//   printf("%s: ", hf->descr);
   if (hf->out_spec == 'm')
   {
      fprint_by_spec(fmd->msg->out_point->fs, (uint8*)&(mf.value.data),
                    mf.size, 'd', 0);
      fprintf(fmd->msg->out_point->fs, ";");
      fprint_by_spec(fmd->msg->out_point->fs, (uint8*)&(mf.value.data),
                    mf.size, 't', 0);
   }
   else
   {
      fprint_by_spec(fmd->msg->out_point->fs,
                     (mf.type == mft_data) ? (uint8*)&(mf.value.data) : mf.value.data_ptr,
                     mf.size, hf->out_spec, 0);
   }
   if (strceq(hf->name, "$crc"))
   {
      fprintf(fmd->msg->out_point->fs, ";%s", (mf.value.data == fmd->msg->crc_real) ? _("correct") : _("incorrect"));
   }
   fprintf(fmd->msg->out_point->fs, ";");
   return(true);
}
//------------------------------------------------------------------------------
static
bool mdi_csv(struct t_message *msg)
// Message data interpreter for human-readable output.
{
   assert(msg != NULL);

   cl_iterate(msg->fields, &mdi_csv_idt, msg, NULL);

   if (msg->remainder_len)
   {
      fprint_hexdump(msg->out_point->fs, msg->pointer, msg->remainder_len, 0);
   }
   return(true);
}
//------------------------------------------------------------------------------
static
bool shm_csv(struct t_full_msgdata *fmd)
// Message structure interpreter for CSV output.
{
   struct t_message           *msg        = fmd->msg;
   struct t_message_raw       *raw_msg    = msg->raw_message;
   uint16                     raw_data_sz = message_data_len(raw_msg);
   FILE     *fs;

   fs = (msg->mmd) ?  msg->out_point->fs : stdout;
   cl_iterate(fmd->pmd->header_fields_md->fields_md, &mdi_csv_ihr, fmd, NULL);
   if (raw_data_sz)
   {
      if (msg->mmd)
      {
         if (!(msg->mmd->is_compound || msg->out_only_headers)) mdi_csv(msg);
      }
      else
      {
         fprint_hexdump(fs, (uint8*)&(raw_msg->data), raw_data_sz, 0);
         fprintf(fs, _("hex data"));
         fprintf(fs, ";");
      }
   }
   else
   {
      fprintf(fs, _("Message has not data;"));
   }

   fprintf(fs, "\n");

   fflush(fs);
   return(true);
}
//------------------------------------------------------------------------------
static
int mdi_raw(struct t_message *msg)
{
   return(fwrite(&(msg->raw_message->data), message_data_len(msg->raw_message),
                 1, msg->out_point->fs));
}
//------------------------------------------------------------------------------
static
bool shm_raw(struct t_full_msgdata *fmd)
// Message structure interpreter for raw output.
{
   uint16   msg_data_sz = message_data_len(fmd->msg->raw_message);
   FILE     *fs;
   uint16   crc         = message_crc(fmd->msg->raw_message);

   fs = (fmd->msg->mmd) ? fmd->msg->out_point->fs : stdout;
   if (fwrite(&(fmd->msg->raw_message->header), message_header_size, 1, fs) < 1)
   {
      fprintf(stderr, _("RAW: Message header writing error [%s]!\n"), strerror(errno));
      return(false);
   }

//   fprintf(fs, "Message data size:\t\t%d bytes\n", msg_data_sz);
   if (msg_data_sz)
   {
      if (fmd->msg->mmd)
      {
         if (!(fmd->msg->mmd->is_compound || fmd->msg->out_only_headers))
            mdi_raw(fmd->msg);
      }
      else
      {
         if (fwrite(&(fmd->msg->raw_message->data), msg_data_sz, 1, fs) < 1)
         {
            fprintf(stderr, _("RAW: Message data writing error!\n"));
            return(false);
         }
      }
   }
   // CRC.
   if (fwrite(&crc, message_crc_size, 1, fs) < 1)
   {
      fprintf(stderr, _("RAW: Message CRC writing error [%s]!\n"), strerror(errno));
      return(false);
   }

   fflush(fs);
   return(true);
}
//------------------------------------------------------------------------------
f_shm_routine shm_table[] =
{
   [mof_prn]   = &shm_prn,
   [mof_csv]   = &shm_csv,
   [mof_raw]   = &shm_raw
};
//------------------------------------------------------------------------------
