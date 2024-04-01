use strict;
use warnings;
use Carp qw[confess];
use Config qw[%Config];
use Archive::Zip qw[];
use lib './lib';

#
my $PROJECT = 'perl4mIRC';
if (   (!-f $PROJECT . $Config{'_o'})
    || ((stat($PROJECT . '.c'))[2] > (stat($PROJECT . $Config{'_o'}))[2]))
{
########################################################### Compile ##########
    confess 'Failed to compile '
        . $PROJECT
        if system(join ' ',
                  $Config{'cc'},
                  '-DBUILDING_DLL=1',
                  $Config{'ccflags'},
                  $Config{'optimize'},
                  (map {qq[-I"$_"]} './', $Config{'incpath'},
                   $Config{'incpath'}, $Config{'archlib'} . '/CORE/'
                  ),
                  '-std=c99',
                  '-c ' . $PROJECT . '.c',
                  '-o ' . $PROJECT . $Config{'_o'}
        );
}
if ((!-f $PROJECT . $Config{'so'})
    || ((stat($PROJECT . $Config{'_o'}))[2]
        > (stat($PROJECT . $Config{'so'}))[2])
    )
{
############################################################## Link ##########
    confess 'Failed to link '
        . $PROJECT
        if system(join ' ',
                  $Config{'ld'},
                  $Config{'lddlflags'},
                  $PROJECT . $Config{'_o'},
                  $Config{'archlib'} . '/CORE/' . $Config{'libperl'},
                  '-Wl,-k,-export-all-symbols -mdll -static -o '
                      . $PROJECT . '.'
                      . $Config{'so'}
        ) || !-f $PROJECT . '.' . $Config{'so'};
##################################################### Zip it all up ##########
    my %files = ('perl4mirc.' . $Config{'so'} => 'perl4mirc.' . $Config{'so'},
                 'CHANGES.txt'                => 'docs/CHANGES.txt',
                 'LICENSE.txt'                => 'docs/LICENSE.txt',
                 'perl4mirc.c'                => 'src/perl4mirc.c',
                 'perl.mrc'                   => 'perl.mrc',
                 'README.txt'                 => 'docs/README.txt',
                 'lib/mIRC.pm'                => 'lib/mIRC.pm'
    );
    my $zip = Archive::Zip->new();
    for my $file (keys %files) {
        $zip->addFile($file, $files{$file})->desiredCompressionLevel(9)
            or warn "Can't add file $file\n";
    }
    my $zip_out = sprintf '%s-%s.zip', $PROJECT, '1.5012002';
    $zip->writeToFileNamed($zip_out);
    warn sprintf '%s => %d bytes', $zip_out, -s $zip_out;

    system qw[copy perl4mIRC.dll "C:\\Program Files (Portable)\\mIRC\\perl4mIRC.dll" /Y];
    #system qw[copy perl.mrc "C:\Users\Sanko Robinson\Downloads\perl4mIRC-1.5012001\perl.mrc" /Y];
    #system qw[copy lib\mIRC.pm "C:\Users\Sanko Robinson\Downloads\perl4mIRC-1.5012001\lib\mIRC.pm" /Y];
}
END{unlink $PROJECT . $Config{'_o'};}

#exit system qw["C:\Users\Sanko Robinson\Downloads\perl4mIRC-1.5012001\mirc.exe" /portable];
