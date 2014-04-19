all: create push

create:
	perl create.pl --repository ~/debian/marpa

push:
	perl ftpreplacedir.pl ftpperso.free.fr jeandamiendurand - ~/debian/marpa debian/marpa
