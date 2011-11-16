-- checks a memory log to ensure that everything that has been allocated has also been deallocated

local mem_blocks = {}

local ln
while true do
  ln = io.read()
  if (ln == nil) then
    break
  end
  if (ln ~= '') then
    local t = string.sub(ln, 1, 5)
    local p = string.sub(ln, 8, 15)
    if (mem_blocks[p] == nil) then
      mem_blocks[p] = { count = 0, ln = 'ROFL' }
    end

    if (t == 'ALLOC') then
      mem_blocks[p]['count'] = mem_blocks[p]['count'] + 1
      mem_blocks[p]['ln'] = ln
    elseif (t == 'DELOC') then
      mem_blocks[p]['count'] = mem_blocks[p]['count'] - 1
    end
  end
end

io.write('Leaked blocks:\n')
table.foreach(mem_blocks, function (k, v)
  if (v.count ~= 0) then
    io.write(v.count .. ': ' .. v.ln .. '\n')
  end
end)

io.write(' --- \n')
