# conditions.rb: utility library for reading/writing "condition" files
# Copyright 2014 by Vincent Fourmond
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
# USA


# A "conditions file", usually conditions.dat is a TAB-separated data
# file that contains meta-data about other files. The first column
# contains the file name (transformed to get rid of suffixes), and all
# the rest contains either text or (more commonly) numerical data
# attached to the files.
#
# Lines starting with # are comments. Before the data per se, there
# should be a line starting with ## that contains the names of the
# columns.
class ConditionsFile

  # Header (ie any comment before the final ## line) (all comments
  # afterwards are ignored)
  attr_accessor :header
  
  # The name of the columns
  attr_accessor :column_names
  
  # The lines, an array of arrays
  attr_accessor :data

  # The file name. It is better that only one instance is loaded ?
  attr_accessor :file_path

  # The directory in which the file is, for the purposes of deciding
  # which is the meta-data for a file.
  attr_accessor :base_directory


  # Normalizes the file name. Strips all extensions from the file
  # name.
  def self.normalize_file_name(file)
    return file.sub(/\.[^\/]+$/, '')
  end


  def has_file?(file)
    fn = File::absolute_path(file)
    bd = "#{@base_directory}/"
    if fn.start_with?(bd)
      fn2 = ConditionsFile::normalize_file_name(fn[bd.size..-1])
      return @index[fn2]
    end
    return false
  end

  # Registers the given file to the global file hash.
  def self.register_file(cond)
    @files ||= {}
    @loaded_files ||= {}
    if @loaded_files[cond.file_path]
      return                    # Or raise ?
    else
      @loaded_files[cond.file_path] = cond
    end
    @files[cond.base_directory] ||= []
    @files[cond.base_directory] << cond
  end

  # Returns the data for the given file, in the form of a hash.
  def file_data(file)
    a = has_file?(file)
    ret = {}
    if a
      1.upto(@column_names.size - 1) do |i|
        ret[@column_names[i]] = a[i]
      end
    end
    return ret
  end

  # Finds among the registered condition files the ConditionsFile that
  # has the given data file.
  #
  # Starts with all the files in the dir in which the file is located,
  # and proceeds up the hierarchy.
  def self.conditions_for(file, autoload = "conditions.dat")
    dir = File::dirname(File::absolute_path(file))
    while true
      files = if @files 
                @files[dir]
              else
                nil
              end
      if files
        for f in files
          if f.has_file?(file)
            return f.file_data(file)
          end
        end
      end
      if autoload
        candidate = "#{dir}/#{autoload}"
        if File::readable?(candidate) && (! @loaded_files || ! @loaded_files[candidate])
          # puts "Trying #{candidate}"
          f = read_file(candidate)
          if f.has_file?(file)
            return f.file_data(file)
          end
        end
      end
      d = File::dirname(dir)
      if d == dir
        break
      end
      dir = d
    end
    return nil
  end





  # # Parse the data, ie make a float out of something that looks like a
  # # float
  # def self.parse_data(txt)
  #   begin
  #     return Float(txt)
  #   rescue
  #     return txt
  #   end
  # end

  # Creates a DataFile, potentially with an index column
  def initialize
    @header = []
    @data = []

    # A index normalized file name -> line 
    @index = {}
  end

  def set_column_names(names)
    @column_names = names
  end

  # Adds a data-line (already split)
  def add_data_line(data)
    # Normalize file names !
    fn = ConditionsFile.normalize_file_name(data[0])
    nd = [fn, *data[1..-1]]

    # nd += data[1..-1].map {|field| 
    #   parse_data(field)
    # }

    @data << nd
    @index[fn] = nd
  end


  # Writes out the data
  def write_file(out = STDOUT, sorted = true)
    if sorted
      @data.sort { |a,b| a[0] <=> b[0] }
    end
    if @header.size > 0
      out.puts @header.join("\n")
    end
    out.puts "## " + @column_names.join("\t")
    for d in @data
      out.puts d.join("\t")
    end
  end

  # Reads a file.
  def read_file(file)
    File.open(file, "r") do |f|
      @file_path = File::absolute_path(file)
      @base_directory = File::dirname(@file_path)
      ConditionsFile::register_file(self)
      intro = true
      while line = f.gets
        line.chomp!
        if line =~ /^##\s*(.*)/
          set_column_names($1.split(/\t/))
          intro = false
          if @intro_end_hook
            @intro_end_hook.call
          end
        elsif intro
          @header << line
        else
          d = line.split(/\t/)
          add_data_line(d)
        end
      end
    end
  end

  # Reads the given file (an IO object or a file name) and returns a
  # new DataFile.
  def self.read_file(file)
    ret = ConditionsFile.new
    ret.read_file(file)
    return ret
  end

end

# for f in ARGV
#   p [f, ConditionsFile::conditions_for(f)]
# end
