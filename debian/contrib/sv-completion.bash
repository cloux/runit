# From: Jérémy Lal <kapouer@melix.org>
# To: Debian Bug Tracking System <submit@bugs.debian.org>
# Subject: runit: bash completion for sv
# Date: Thu, 06 Jun 2013 14:00:39 +0200
# 
# Please find attached a bash-completion file for sv.

_sv_services()
{
    echo $(ls /etc/service/)
}

_sv_commands()
{
    echo "status up down once pause cont hup alarm interrupt quit 1 2 term kill exit sv start stop reload restart shutdown force-stop force-reload force-restart force-shutdown try-restart check"
}

_sv()
{
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}
    if [ $COMP_CWORD -eq 1 ]; then
        COMPREPLY=( $( compgen -W "$(_sv_commands)" $cur ) )
    elif [ $COMP_CWORD -eq 2 ]; then
        COMPREPLY=( $( compgen -W "$(_sv_services)" $cur ) )
        _filedir -d
    fi
}

complete -F _sv sv
