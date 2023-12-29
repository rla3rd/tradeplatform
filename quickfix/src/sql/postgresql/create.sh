#dropdb -U postgres -q quickfix
#createdb -U postgres quickfix
psql -U indie -d quickfix -f postgresql.sql
