//------------------------------------------------------------------------------
static __inline bool test_link_type(struct t_message_tree_node *node,
                                    const size_t               types_count,
                                    ...) 
{
   register size_t      i;
   register t_link_type lt;
   va_list              ap;

   if (!node) return(false);
   if (node->type != nt_link) return(false);

   va_start(ap, types_count);
   for (i = 0; i < types_count; i++)
   {
      lt = va_arg(ap, t_link_type);
      if (lt == node->type)
      {
         va_end(ap);
         return(true);
      }
   }
   va_end(ap);
   return(false);
}
//------------------------------------------------------------------------------
/*
 * //                                      16   12   5
 * // this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 * // This works out to be 0x1021, but the way the algorithm works
 * // lets us use 0x8408 (the reverse of the bit pattern).  The high
 * // bit is always assumed to be set, thus we only use 16 bits to
 * // represent the 17 bit value.
 * */
#define CRC16_POLY 0x1021
const uint16 crc16(uint8 *buf, register uint16 len)
{
   unsigned char i;
   uint32 data;
   uint32 crc = 0xffff;

   if (len== 0) return (~crc);
   do
	{
      for (i = 0, data = (uint32)0xff & *buf++; i < 8; i++, data >>= 1)
      {
         if ((crc & 0x0001) ^ (data & 0x0001)) crc = (crc >> 1) ^ CRC16_POLY;
         else  crc >>= 1;
      }
   } while (--len);

//   crc   = ~crc;
   data  = crc;
   crc   = (crc << 8) | (data >> 8 & 0xff);

   return(crc);
}
//------------------------------------------------------------------------------
FILE *fopen_def_sock(char *subopt_str, struct sockaddr_in *m_addr)
// Return socket.
{
   char                 *subopt_val = NULL;
   char                 *sos        = subopt_str;
   char                 *address    = NULL;
   unsigned short       port        = PORT;
   bool                 subopt_err  = false;

   enum
   {
      SOPT_ADDR = 0,
      SOPT_PORT,
   };
   
   char *const subopt_token[] =
   {
      [SOPT_ADDR]       = "ip",
      [SOPT_PORT]       = "port",
      NULL
   };

   if (!m_addr) return(NULL);

   if (sos)
   while (!subopt_err && (*sos != '\0'))
   {
      switch (getsubopt(&sos, subopt_token, &subopt_val))
      {
         case SOPT_ADDR:
            if (!subopt_val)
            {
               fprintf(stderr, "Missing IP address!\n");
               subopt_err = true;
               continue;
            }
            dfree(address);
            if ((address = strdup(subopt_val)) == NULL)
            {
               fprintf(stderr, "Can't allocate memory for IP address!\n");
               subopt_err = true;
               continue;
            }
         break;
         case SOPT_PORT:
            if (!subopt_val)
            {
               fprintf(stderr, "Missing port value!\n");
               subopt_err = true;
               continue;
            }
            port = atoi(subopt_val);
         break;
      }
   }
   //!!! dfree(subopt_val);
   if (subopt_err) return(NULL);

   if (!address) address = strdup(ADDR);
   
   memzero(m_addr, sizeof(struct sockaddr_in));
   m_addr->sin_family = AF_INET;
   m_addr->sin_port   = htons(port);

   if (inet_pton(AF_INET, address, &(m_addr->sin_addr)) == 0)
   {
      fprintf(stderr, "Address resolution error [%s]!\n", strerror(errno));
      dfree(address);
      return(NULL);
   }
   dfree(address);

   return(fopen_dgram_socket(m_addr, "rb"));
}
//------------------------------------------------------------------------------
switch (opt)
      {
         case 'c':
            config_file = strdup(optarg);
         break;
         case 'n':
            if (config.def_in.type != tiop_unknown)
            {
               fprintf(stderr, "Options -n and -l is not compatible!\n");
               exit(EXIT_FAILURE);
            }
            if (!start_network(&config, optarg)) exit(EXIT_FAILURE);
         break;
         case 'l':
            if (config.def_in.type != tiop_unknown)
            {
               fprintf(stderr, "Options -n and -l is not compatible!\n");
               exit(EXIT_FAILURE);
            }
            if (!optarg)
            {
               config.def_in.type   = tiop_stream;
               config.def_in.fs     = stdin;
            }
            else 
            {
               config.def_in.type = tiop_file;
               if (((config.def_in.file.filename = realpath(optarg, NULL)) == NULL) ||
                   !init_io_point(&config.def_in, true))
               {
                  fprintf(stderr, "Can't open input file %s\n!", optarg);
                  exit(EXIT_FAILURE);
               }

            }
         break;

//-----------------------------------------------------------------------
   if ((fl_base->def_in = init_def_iop(def_in)) == NULL)
   {
      assert(destroy_filters_db(filters, fl_base));
      return(NULL);
   }
   if (!add_io_point(fl_base->sources, fl_base->def_in))
   {
      dfree(fl_base->def_in);
      assert(destroy_filters_db(filters, fl_base));
      return(NULL);
   }
   if ((fl_base->def_out = init_def_iop(def_out)) == NULL)
   {
      assert(destroy_filters_db(filters, fl_base));
      return(NULL);
   }
   if (!add_io_point(fl_base->dests, fl_base->def_out))
   {
      dfree(fl_base->def_out);
      assert(destroy_filters_db(filters, fl_base));
      return(NULL);
   }
//------------------------------------------------------------------------------
#define ST_CHAN_ADDR(sts)     (sts & 0xfff0)
#define ST_CHAN_STATUS(sts)   (sts & 0x000f)
//------------------------------------------------------------------------------


               case op_div:
               break;
               case op_mod:
               break;
               case op_plus:
               break;
               case op_lshift:
               break;
               case op_rshift:
               break;
               case op_lt:
               break;
               case op_gt:
               break;
               case op_ge:
               break;
               case op_le:
               break;
               case op_eq:
               break;
               case op_ne:
               break;
               case op_bw_and:
               break;
               case op_bw_xor:
               break;
               case op_bw_or:
               break;
               case op_log_and:
               break;
               case op_log_or:
                  cur_spos = term9(cur_spos, result);
               break;
               case op_size:
               break;
               case op_offset:
               break;
/*   switch (field->field_md->out_spec)
{
   case 'x':
      if ((field->real_size * 2) > MAX_PRN_LEN)
      {
         fputc('\n', msg->mmd->out_file);
         fprint_hexdump(msg->mmd->out_file, field->field_data, field->real_size, DUMP_WLIMIT);
      }
      else
      {
         data_value = 0;
         memcpy(&data_value, field->field_data, field->real_size);
         fprintf(msg->mmd->out_file, "0x%0*X ", field->real_size * 2, (unsigned int)data_value);
      }
      fputc('\n', msg->mmd->out_file);
   break;
   case 'o':
   
   break;
   case 'u':
      data_value = 0;
      memcpy(&data_value, field->field_data, field->real_size);
      fprintf(msg->mmd->out_file, "%u\n", (unsigned int)data_value);
   break;
   case 'd':
      data_value = 0;
      memcpy(&data_value, field->field_data, (uint16)field->real_size);
      fprintf(msg->mmd->out_file, "%lld\n", (signed long long int)data_value);
   break;
   case 'b':
      if ((len * 8) > MAX_PRN_LEN) fputc('\n', msg->mmd->out_file);
      fprintf(msg->mmd->out_file, "0b");
      fprint_bin(msg->mmd->out_file,field->field_data, field->real_size, DUMP_WLIMIT);
      fputc('\n', msg->mmd->out_file);
   break;
   case 't':
      tm = *(time_t*)(time32*)field->field_data;
      fprintf(msg->mmd->out_file, "%d -> %s", *((time32*)(field->field_data)),
         (*(time32*)(field->field_data)) ? asctime(gmtime(&tm)) : "not configured\n");
   break;
   case 'c':
      for (len = 0; (len < field->real_size) && (field->field_data[len] != 0); len++)
         fprintf(msg->mmd->out_file, "%c", field->field_data[len]);
      fputc('\n', msg->mmd->out_file); 
   break;
   case 'n':
      // Invisible field.
   break;
   default:
      fprintf(stderr, "Unknown output format specifier: \'%c\'\n", field->field_md->out_spec);
   break;
}*/


/*   switch (field->field_md->out_spec)
   {
      case 'x':
         data_value = 0;
         fprintf(msg->mmd->out_file, "0x");
         for (i = 0; i < field->real_size; i++)
            fprintf(msg->mmd->out_file, "%02X", ((unsigned char*)(field->field_data))[i]);
         fputc(';', msg->mmd->out_file);
      break;
      case 'o':
      
      break;
      case 'u':
         data_value = 0;
         memcpy(&data_value, field->field_data, field->real_size);
         fprintf(msg->mmd->out_file, "%u;", (unsigned int)data_value);
      break;
      case 'd':
         data_value = 0;
         memcpy(&data_value, field->field_data, (uint16)field->real_size);
         fprintf(msg->mmd->out_file, "%lld;", (signed long long int)data_value);
      break;
      case 'b':
         fputc('b', msg->mmd->out_file);
         fprint_bin(msg->mmd->out_file, field->field_data, field->real_size, 0);
         fputc(';', msg->mmd->out_file);
      break;
      case 't':
         tm = *(time_t*)(time32*)field->field_data;
         strftime(ta, 255, "%D;%T", gmtime(&tm));
         fprintf(msg->mmd->out_file, "%s;",  (strlen(ta)) ? ta : "not configured");
      break;
      case 'c':
         for (len = 0; (len < field->real_size) && (field->field_data[len] != 0); len++)
            fprintf(msg->mmd->out_file, "%c", field->field_data[len]);
         fputc(';', msg->mmd->out_file); 
      break;
      case 'n':
         // Invisible field.
      break;
      default:
         fprintf(stderr, "Unknown output format specifier: \'%c\'\n", field->field_md->out_spec);
      break;
   }*/



/*   fprintf(fs, "Message length from header:%*c0x%04X (%d) bytes\n",
      3, ' ',
      raw_msg->header.message_length, raw_msg->header.message_length);
   fprintf(fs, "Message type:%*c0x%04X ", 17, ' ', raw_msg->header.type);
   if (msg->mmd && msg->mmd->message_descr) fprintf(fs, "(%s)\n", msg->mmd->message_descr);
   else fprintf(fs, "(unknown type)\n");

   fprintf(fs, "Message code:%*c0x%02X\n", 17, ' ', MESSAGE_CODE(raw_msg));
   fprintf(fs, "Version number:%*c0x%02X\n", 15, ' ', MESSAGE_VERSION(raw_msg));

   fprintf(fs, "Reciever:%*c0x%04X (%s)\nSender:%*c0x%04X (%s)\n", 
      21, ' ',
      raw_msg->header.reciever, get_station_name_by_addr(raw_msg->header.reciever), 
      23, ' ',
      raw_msg->header.sender, get_station_name_by_addr(raw_msg->header.sender)
   );
   fprintf(fs, "Time:%*c%d -> %s", 25, ' ',
      raw_msg->header.time, asctime(gmtime(&tm)));
   fprintf(fs, "Id:%*c%d\nReserved:%*c%d\n", 27, ' ',
      raw_msg->header.id, 21, ' ', raw_msg->header.reserved);
   if (message_crc(raw_msg) == msg->crc_real)
   {
      fprintf(fs, "CRC:%*c0x%04X [correct]\n",
           26, ' ', message_crc(raw_msg));
   }
   else
   {
      fprintf(fs, "CRC:%*c0x%04X [Incorrect! Calculated CRC: 0x%04X]\n",
           26, ' ', message_crc(raw_msg), msg->crc_real);
   }

   fprintf(fs, "Message data size:%*c%d bytes\n", 12, ' ', raw_data_sz);*/



   // Message length from header.
/*   fprintf(fs, "%d;", msg->raw_message->header.message_length);

   // Message type
   fprintf(fs, "0x%04X;", msg->raw_message->header.type);
   if (msg->mmd && msg->mmd->message_descr) fprintf(fs, "%s;", msg->mmd->message_descr);
   else fprintf(fs, "unknown type;");

   // Message code
//   fprintf(fs, "Message code:\t\t\t0x%02X\n", MESSAGE_CODE(msg));
//   fprintf(fs, "Version number:\t\t\t0x%02X\n", MESSAGE_VERSION(msg));

   // Reciever;Sender
   fprintf(fs, "0x%04X;%s;0x%04X;%s;", 
      msg->raw_message->header.reciever, get_station_name_by_addr(msg->raw_message->header.reciever), 
      msg->raw_message->header.sender, get_station_name_by_addr(msg->raw_message->header.sender)
   );
   // Time
   strftime(ta, sizeof(ta), "%D;%T", (gmtime(&tm)));
   fprintf(fs, "%s;", ta);
   // Id;Reserved;CRC,
   fprintf(fs, "%d;%d;0x%04X;%s;",
      msg->raw_message->header.id, msg->raw_message->header.reserved,
      message_crc(msg->raw_message),
      (message_crc(msg->raw_message) == msg->crc_real) ? "correct" : "incorrect");
//   fprintf(fs, "Message data size:\t\t%d bytes\n", msg_data_sz);*/

