local ok, err = pcall(require, 'blueprints.pet_nodes')
if not ok then
    print('[PetBlueprintEntry] Warning: pet_nodes load failed: ' .. tostring(err))
end