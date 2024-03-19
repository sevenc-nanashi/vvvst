require "json"
require "open3"
require "http"

def generate(name, license:, url:)
  path = name.split("/").last
  {
    name: name,
    version: Open3.capture2("git", "rev-parse", "HEAD", chdir: "Depends/#{path}").first.strip,
    license: license,
    text: HTTP.get(url).to_s
  }
end

licenses = [
  generate(
    "juce-framework/JUCE",
    license: "GPL-3.0",
    url:
      "https://raw.githubusercontent.com/juce-framework/JUCE/master/LICENSE.md"
  ),
  generate(
    "Tracktion/choc",
    license: "MIT",
    url: "https://raw.githubusercontent.com/Tracktion/choc/main/LICENSE.md"
  ),
  generate(
    "lasselukkari/MimeTypes",
    license: "MIT",
    url:
      "https://raw.githubusercontent.com/lasselukkari/MimeTypes/master/LICENSE"
  ),
  { name: "WebView2Loader.dll", version: "-", license: "-", text: <<~LICENSE }
    Copyright (C) Microsoft Corporation. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    * The name of Microsoft Corporation, or the names of its contributors
    may not be used to endorse or promote products derived from this
    software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
LICENSE
]

File.write("licenses.json", JSON.pretty_generate(licenses))
