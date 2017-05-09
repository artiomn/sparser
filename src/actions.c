//------------------------------------------------------------------------------
#include "actions.h"
//------------------------------------------------------------------------------
#ifdef _WIN32
   #include <winuser.h>
#endif
//------------------------------------------------------------------------------
struct t_sized_data
{
   char     *data_ptr;
   char     out_spec;
   size_t   size;
};
//------------------------------------------------------------------------------
static bool gf_iterator(void *hf, void *nm)
{
   assert(hf != NULL);
   assert(nm != NULL);

   if (strceq(((struct t_header_field*)hf)->name, (char*)nm)) return(false);
   return(true);
}
//------------------------------------------------------------------------------
static
bool get_field(struct t_sized_data        *dest,
               struct t_full_msgdata      *fmd,
               const char                 *field_name)
//!!!INCORRECT. replace static mfl.data with buffer!
{
   
   static struct t_metafield  mfl;
   struct t_message_fli       *fl;
   struct t_header_field      *hfl;
   struct t_parser_metadata   *pmd;

   assert(fmd                    != NULL);
   assert(field_name             != NULL);
   assert(dest                   != NULL);

   pmd = fmd->pmd;

   assert(pmd                    != NULL);
   assert(pmd->header_fields_md  != NULL);

   if (is_first_namesym(field_name[0]))
   {
      if ((fl = get_field_by_name(field_name, fmd->msg)) == NULL) return(false);
      dest->data_ptr = fl->field_data;
      dest->size     = fl->real_size;
      dest->out_spec = fl->field_md->out_spec;
      return(true);
   }
   else if (is_metafield_fs(field_name[0]))
   {
      mfl.name = field_name;
      if (!(mfl.name && get_metafield_by_name(fmd, &mfl)))
      {
         return(false);
      }

      if ((hfl = (struct t_header_field*)cl_find(pmd->header_fields_md->fields_md, gf_iterator, field_name)) != NULL)
      {
         dest->out_spec = hfl->out_spec;
      }
      else dest->out_spec = 'x';

      if (mfl.type == mft_data)
      {
         dest->data_ptr = (char*)&(mfl.value.data);
         dest->size     = mfl.size;
         return(true);
      }
      else if (mfl.type == mft_ptr)
      {
         dest->data_ptr = mfl.value.data_ptr;
         dest->size     = mfl.size;
         return(true);
      }
      else
      {
         assert("Bad metafield!" && false);
      }
   }

   return(false);
}
//------------------------------------------------------------------------------
static
bool replace_field_content(char                       **act_par,
                           struct t_full_msgdata      *fmd,
                           long int                   *dest_sz,
                           char                       **result,
                           long int                   *res_pos)
{
   char                 *id_start   = NULL;
   char                 *field_str  = NULL;
   long int             field_nm_sz = 0;
   struct t_sized_data  fl;

   assert(act_par    != NULL);
   assert(*act_par   != NULL);
   assert(dest_sz    != NULL);
   assert(result     != NULL);
   assert(*result    != NULL);
   assert(res_pos    != NULL);

   id_start = *act_par;

   if (is_first_namesym(**act_par))
   {
      (*act_par)++;
   }
   else if (is_metafield_fs(**act_par))
   {
      (*act_par)++;
   }
   else
   {
      // Error.
      return(false);
   }
   while (is_namesym(**act_par)) (*act_par)++;
   field_nm_sz = *act_par - id_start;
   if ((field_str = strndup(id_start, field_nm_sz)) == NULL)
   {
      return(false);
   }
   if (!get_field(&fl, fmd, field_str))
   {
      dfree(field_str);
      return(false);
   }
   dfree(field_str);

   if (fl.size == 0) return(true);

   if ((field_str = sprintf_by_spec(fl.data_ptr, fl.size, fl.out_spec)) == NULL)
      return(false);
   fl.data_ptr = field_str;
   fl.size     = strlen((char*)fl.data_ptr);
   if (fl.size > 0)
   {
      field_nm_sz = fl.size - field_nm_sz - 1;

      if (field_nm_sz >= 0)
      {
         *dest_sz += field_nm_sz;
         if ((id_start = realloc(*result, *dest_sz)) == NULL)
         {
            dfree(fl.data_ptr);
            return(false);
         }
         *result = id_start;
      }
      memcpy(*result + *res_pos, fl.data_ptr, fl.size);
      *res_pos += fl.size;
   }
   dfree(fl.data_ptr);

   return(true);
}
//------------------------------------------------------------------------------
static
char* make_act_str(struct t_full_msgdata  *fmd,
                   char                   *act_par)
// Allocate memory and return parsed action string.
// Memory must be deallocated somewhere.
{
   long int dest_sz     = strlen(act_par) * sizeof(char) + sizeof(char);
   char     *act_str    = dmalloc(dest_sz);
   bool     err_flag    = false;
   bool     pref_flag   = false;
   long int i           = 0;

   assert(fmd        != NULL);
   assert(fmd->pmd   != NULL);

   memzero(act_str, dest_sz);

   while (!err_flag && act_par && *act_par != '\0')
   {
      if (!pref_flag)
      {
         if (*act_par != '%')
         {
            act_str[i++] = *act_par;
         }
         else
         {
           pref_flag = true;
         }
         act_par++;
      }
      else
      {
         if (*act_par == '%')
         {
            act_str[i++] = '%';
            act_par++;
         }
         else
         {
            if (!replace_field_content(&act_par, fmd,
                                       &dest_sz, &act_str, &i))
            {
               err_flag = true;
               break;
            }

         }
         pref_flag = false;
      }
   }

   if (err_flag)
   {
      dfree(act_str);
      return(NULL);
   }

   act_str[i] = '\0';
   return(act_str);
}
//------------------------------------------------------------------------------
static
bool aci_system(struct t_full_msgdata     *fmd,
                struct t_filter_action    *action)
{
   char *act_str;

   assert(action != NULL);

   act_str = make_act_str(fmd, action->params);

   if (act_str)
   {
      #if DEBUG_LEVEL >= 2
      fprintf(stderr, "Executing command: %s\n", act_str);
      #endif
      system(act_str);
      dfree(act_str);
      return(true);
   }

   return(false);
}
//------------------------------------------------------------------------------
bool show_message(struct t_message *msg, const char *msg_str)
{
   #ifdef _WIN32
      MessageBox(0, msg, PACKAGE_NAME,
                 MB_OK |
                 (msg->error_level > 0) ? MB_ICONEXCLAMATION : MB_ICONINFORMATION);
   #else
      fprintf(stderr, "%s\n", msg_str);
   #endif
}
//------------------------------------------------------------------------------
static
bool aci_message(struct t_full_msgdata    *fmd,
                 struct t_filter_action   *action)
{
   char *act_str;

   assert(action  != NULL);
   assert(fmd     != NULL);

   act_str = make_act_str(fmd, action->params);

   if (act_str)
   {
      show_message(fmd->msg, act_str);
      dfree(act_str);
      return(true);
   }

   return(false);
}
//------------------------------------------------------------------------------
f_action_routine actions_table[] =
{
   [fat_system]   = &aci_system,
   [fat_msg]      = &aci_message,
   [fat_write]    = NULL
};
//------------------------------------------------------------------------------

