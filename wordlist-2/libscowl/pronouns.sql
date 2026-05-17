
with wds as (select w.*,coalesce(variant_level,0) as variant_level from words w left join variant_info using (word_id))
select * from      (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pn0 from wds where pos = 'pn0' group by lemma_id) pn0
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pn1 from wds where pos = 'pn1' group by lemma_id) pn1 using (lemma_id)
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pns from wds where pos = 'pns' group by lemma_id) pns using (lemma_id)
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pnd from wds where pos = 'pnd' group by lemma_id) pnd using (lemma_id)
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pnp from wds where pos = 'pnp' group by lemma_id) pnp using (lemma_id)
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pnr0 from wds where pos = 'pnr0' group by lemma_id) pnr0 using (lemma_id)
         left join (select lemma_id, group_concat(distinct word order by entry_rank,variant_level,word) as pnrs from wds where pos = 'pnrs' group by lemma_id) pnrs using (lemma_id)
         order by pn0
;
