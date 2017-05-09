#! /bin/sh

if [ ! -d gnu_lib ]; then
   echo "Customizing GNU LIB"
   gnulib-tool --aux-dir=gnu_lib --source-base=gnu_lib/lib --m4-base=gnu_lib/m4 \
      --tests-base=gnu_lib/tests --import malloc strndup inet_pton getsubopt inet_ntop canonicalize-lgpl setenv gettext #time_r 
elif [ x"$1" = "x-u" ]; then
   echo "Updating GNU LIB"
   gnulib-tool --update
fi

touch README AUTHORS NEWS ChangeLog

if [ ! -f ./configure ]; then
   echo "Initial configuration..."
   ( aclocal --force -Im4 -Ignu_lib/m4 && autoconf ) || exit 1
   automake --add-missing --copy --foreign 
else
   echo "Reconfiguration..."
   ( autoreconf -i -Im4 -Ignu_lib/m4 && autoheader ) || exit 1
fi



#&& gettextize -f --intl \
#   && aclocal -I m4 -I gnu_lib/m4 #&& ln -s /usr/share/gettext/gettext.h src

