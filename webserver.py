from http.server import HTTPServer, BaseHTTPRequestHandler
from os.path import join as pjoin
from os import curdir

class helloHandler(BaseHTTPRequestHandler):
    store_path = pjoin(curdir, 'data.txt')

    def do_POST(self):
        content_len = int(self.headers.get('Content-Length'))
        post_body = self.rfile.read(content_len)
        with open(self.store_path, 'a') as fh:
            fh.write(post_body.decode())
        self.send_response(200)

    def do_GET(self):
        if self.path =='/':
            self.path ='/index.html'
        try:
            file_to_open = open(self.path[1:]).read()
            self.send_response(200)
        except:
            file_to_open = "file not found"
            self.send_response(404)
        self.end_headers()

        self.wfile.write(bytes(file_to_open, 'utf-8'))


def main():
    PORT = 8000
    server = HTTPServer(("0.0.0.0", PORT), helloHandler)
    print('Server running on port %s' % PORT)
    server.serve_forever()

if __name__ == '__main__':
    main()