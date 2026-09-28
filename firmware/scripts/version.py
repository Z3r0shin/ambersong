#  AMBERSONG  -  build-time version stamping
#
#  Author's convention, used unchanged:  v.X.YYYYMMDDTHHMMSS
#
#  Stamped at BUILD time rather than kept in a file, because the number that
#  matters is "which binary is this", not "which source revision". Two builds of
#  identical source get different stamps on purpose - the S3 can flash the A32,
#  so the pair WILL run mismatched builds at some point and the handshake has to
#  be able to say which is which.
#
#  MAJOR is the author's release number and nothing else - bumped by hand, only
#  when he declares a release. A protocol break is PROTO_VERSION's job (proto.h),
#  not MAJOR's.
#
#  THE COMMIT, SEPARATELY. The stamp says which binary; FW_COMMIT says which
#  source: the short git hash, with "-dirty" when tracked files had uncommitted
#  changes (the hash alone would then name code that is not what was built),
#  or "nogit" when git could not be asked.
#  It is NOT in the version string, because that string travels in the link
#  handshake in a 24-byte field (PROTO_VERSION_LEN) and the pair would not fit;
#  widening the field would be a protocol change. So FW_COMMIT is shown next
#  to the version (portal, console, settings file, the A32's boot report)
#  and never goes on the wire as part of it.

import datetime
import subprocess

Import("env")

#  Author's rule, 2026-09-02: v.1 is when it is done - fully calibrated, OTA
#  confirmed, and every outstanding problem resolved. Declared done 2026-09-25.
MAJOR = 1

stamp = datetime.datetime.now().strftime("%Y%m%dT%H%M%S")
version = "v.%d.%s" % (MAJOR, stamp)

def git(*args):
    try:
        return subprocess.check_output(["git"] + list(args), cwd=env.subst("$PROJECT_DIR"),
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""

commit = git("rev-parse", "--short=7", "HEAD") or "nogit"
#  Untracked files do not count: only changes to tracked sources make the
#  build differ from the commit it names.
if commit != "nogit" and git("status", "--porcelain", "--untracked-files=no"):
    commit += "-dirty"

env.Append(CPPDEFINES=[("FW_VERSION", '\\"%s\\"' % version),
                       ("FW_COMMIT",  '\\"%s\\"' % commit)])
print("Ambersong firmware version: %s (%s)" % (version, commit))
