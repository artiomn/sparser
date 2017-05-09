#!/bin/sh
awk '
   /^Malloc/ {
      m_count++;
      next;
   }
   /^Free/ {
      f_count++;
      next
   }
   END {
      printf("M: %d\nF: %d\nCOUNTER: %d\n", m_count, f_count, m_count - f_count);
   }' "$1"
