import requests
import sys
from lxml import html

if len(sys.argv) != 2:
    print(f'Usage: {sys.argv[0]} [path to qr image]')
    print('\tDecode the qr image using Zxing\'s online service (the image will be sent to the server)')
    print('\tand extract the relevant fields from the answer')
    sys.exit(0)
filename = sys.argv[1]

# Decode the image using Zxing's website
try:
    req = requests.post('https://zxing.org/w/decode', files={'f': open(filename, 'rb')})
    doc = html.fromstring(req.content)
except:
    print('Error connecting to zxing.org!')
    sys.exit(1)

# Find the decoded text within the result we've got from Zxing
try:
    decodedUrl = doc.xpath('//table[@id="result"]/tr[1]/td/pre')[0].text
except:
    print('Could not find the decoded text within Zxing\'s response!')
    sys.exit(1)

# Extract the relevant parameters
def getParam(url, paramName):
    pos = url.find(f'{paramName}=')
    value = url[pos + len(paramName) + 1: url.find('&', pos)]
    return value

pro2name = {
        'AtermWR8165N-ST': 'WR8165N' # the only device name we know so far
}

pro = getParam(decodedUrl, "Pro")
if pro not in pro2name:
    print(f'Unrecognized device name: {pro}!')
    sys.exit(-1)
print(f'MAC={getParam(decodedUrl, "MAC")}')
print(f'HW_RANDOM_KEY={getParam(decodedUrl, "T")}')
print(f'DEVICE_NAME={pro2name[pro]}')
