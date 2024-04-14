require "bundler/inline"

gemfile do
  source "https://rubygems.org"
  gem "http", "~> 5.2"
  gem "rake", "~> 13.0"
  gem "os", "~> 1.1"
end

task "installer" do
  base = File.read("./Assets/installer_base.nsi")
  base.gsub!("{version}", File.read("version.txt").strip)
  File.write(
    "./build/voicevox_license.md",
    HTTP.get(
      "https://raw.githubusercontent.com/VOICEVOX/voicevox_resource/main/editor/README.md"
    ).to_s,
    mode: "wb",
    encoding: OS.windows? ? "shift_jis" : "utf-8"
  )
  File.write("installer.nsi", base)

  sh 'makensis /INPUTCHARSET UTF8 .\installer.nsi'
end

task "license" do
  require "json"
  require "open3"

  def generate(name, version: nil, license:, url:)
    path = name.split("/").last
    {
      name: name,
      version:
        version ||
          Open3
            .capture2("git", "rev-parse", "HEAD", chdir: "Depends/#{path}")
            .first
            .strip,
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
      "COx2/audio-plugin-web-ui",
      version: "1a42aa9d26e2d433a546555feb271601a87a9cf9",
      license: "MIT",
      url:
        "https://raw.githubusercontent.com/COx2/audio-plugin-web-ui/main/LICENSE"
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
    generate(
      "VST 3 SDK",
      license: "GPL-3.0",
      version: "-",
      url: "https://www.gnu.org/licenses/gpl-3.0.txt"
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

  File.write("Assets/licenses.json", JSON.pretty_generate(licenses))

  puts "Generated licenses.json"
end
