all: create push

create: perl make.pl --verbose --repository ~/debian/marpa

push: perl ftpreplacedir.pl ftpperso.free.fr jeandamiendurand - ~/debian/marpa debian/marpa
