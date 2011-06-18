"""
Simple script that sends the next revision to CIA.vc
flaviojs @ 20110618 (Python 2.7.2)

Requires:
    pysvn
    xmlrpclib

Based on:
    http://cia-vc.googlecode.com/svn/trunk/cia/apps/repos/svn.py
    http://cia-vc.googlecode.com/svn/trunk/cia/htdocs/clients/svn/ciabot_svn.py
"""
import urllib2
import re
import pysvn
import xmlrpclib


# settings
generator_name = 'CIAUpdate'
generator_version = '20110618'
project = 'eAthena'
xmlurl = 'http://cia.vc/stats/project/eAthena/.xml'
svnurl = 'http://eathena-project.googlecode.com/svn/'
revurl = 'http://code.google.com/p/eathena-project/source/detail?r=%(revision)s'
fileurl = 'http://code.google.com/p/eathena-project/source/browse/%(path)s?r=%(revision)s'
rpcurl = 'http://cia.vc'

actionMap = {
    'M': 'modify',
    'A': 'add',
    'D': 'remove',
    }


def get_next_revision():
    req = urllib2.Request(xmlurl)
    response = urllib2.urlopen(req)
    xml = response.read()
    p = re.compile(r'<revision>(\d+)</revision>')
    revs = [int(rev) for rev in p.findall(xml)]
    if len(revs) == 0:
        return 0 # no revisions
    revs.sort()
    last = revs[0]
    for rev in revs[1:]:
        if last+1 < rev:
            return last+1 # missing revision
        last = rev
    return last+1 # next revision

def _ssl_server_trust_prompt(trust_dict):
    retcode = True
    accepted_failures = ~0
    save = False
    return retcode, accepted_failures, save

def get_svn_info(client, url):
        """Return a dictionary of info about a single repository URL.
           May raise a pysvn.ClientError or a forms.ValidationError.
           """
        items = client.info2(url, recurse=False)
        if items:
            return items[0][1]

        # I'm not sure why this happens yet, but some repositories
        # return an empty info dict. We can work around this by
        # picking an arbitrary child item and getting info from that
        # child.
        for child in client.ls(location):
            items = client.info2(child['name'], recurse=False)
            if items:
                return items[0][1]

        raise "Can't retrieve repository info"

def get_current_revision(client):
    info = get_svn_info(client, svnurl)
    if info['kind'] != pysvn.node_kind.dir:
        raise "Must be a directory"
    return info['rev'].number

def escape_to_xml(text, is_attrib=False):
    text = unicode(text)
    text = text.replace("&", "&amp;")
    text = text.replace("<", "&lt;")
    text = text.replace(">", "&gt;")
    if is_attrib == True:
        text = text.replace("'", "&apos;")
        text = text.replace("\"", "&quot;")
    return text

def xmltag(name,contents,attrs={}):
    attr_string = ''.join([' %s="%s"' % (k, escape_to_xml(v,True)) for k,v in attrs.items()])
    return '<%s%s>%s</%s>' % (name, attr_string, escape_to_xml(contents), name)




# get next revision
print 'Getting next revision to send...'
next_revision = get_next_revision()
print 'Next Revision:', next_revision


# get current revision
print 'Getting current revision of svn...'
client = pysvn.Client()
client.callback_ssl_server_trust_prompt = _ssl_server_trust_prompt
current_revision = get_current_revision(client)
print 'Current Revision:', current_revision


if current_revision >= next_revision:
    # generate message
    print 'Generating message...'
    data = {}
    changes = client.log(svnurl,
               revision_start = pysvn.Revision(pysvn.opt_revision_kind.number, next_revision),
               discover_changed_paths = True,
               limit = 1
               )
    generator = '<generator>%s%s</generator>' % (xmltag('name',generator_name),
                                                 xmltag('version',generator_version)
                                                 )
    source = "<source>%s</source>" % (xmltag('project',project))
    for change in changes:
        timestamp = int(change.date)
        data['timestamp'] = timestamp
        timestamp = xmltag('timestamp',timestamp)
        author = change.author
        data['author'] = author
        revision = change.revision.number
        data['revision'] = revision
        files = ['<files>']
        changed_paths = change.changed_paths
        for changed_path in changed_paths:
            path = changed_path.path
            if path.startswith('/'):
                path = path[1:]
            data['path'] = path
            action = changed_path.action
            data['action'] = action
            attrs = {
                'uri': fileurl % data,
                'action': actionMap.get(action)
                }
            f = xmltag('file',path,attrs)
            files.append(f)
        files.append('</files>')
        files = ''.join(files)
        body = '<body><commit>%s%s%s%s%s</commit></body>' % (xmltag('revision',revision),
                                                           xmltag('author',author),
                                                           xmltag('url',revurl % data),
                                                           xmltag('log',change.message),
                                                           files
                                                           )
        message = '<message>%s%s%s%s</message>' % (generator, source, timestamp, body)
        print message
        # send message
        print 'Sending message...'
        if rpcurl.startswith('http:') or rpcurl.startswith('https:'):
            # Deliver over XML-RPC
            xmlrpclib.ServerProxy(rpcurl).hub.deliver(message)

print 'Done'
