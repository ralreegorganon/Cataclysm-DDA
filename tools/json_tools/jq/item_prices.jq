# Output anything with a price

# I do the following to generate a CSV:
#
# echo 'type,id,abstract,price,price_postapoc,file' > prices.csv && \
# jq -fr tools/json_tools/jq/item_prices.jq data/**.json >> prices.csv && \
# csvsort prices.csv -c "type,id,abstract" > prices2.csv

.[]
| select(.price?)?
| {type: .type, id: .id, abstract: .abstract, price: .price, price_postapoc: .price_postapoc, file: input_filename}
| [.[]] 
| @csv