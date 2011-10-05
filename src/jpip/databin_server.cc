#include "databin_server.h"
#include "data/file_segment.h"


namespace jpip
{

  bool DataBinServer::Reset(const ImageIndex::Ptr image_index)
  {
    files.clear();
    metareq = false;
    has_woi = false;
    im_index = image_index;

    /****/
    has_len = false;
    bytes_per_frame = -1;
    sum = 0;
    //delete []woi_composer;
    cout << "[Reset]" << endl;
    /****/

    file = File::Ptr(new File());

    if(!file->OpenForReading(im_index->GetPathName())) return false;
    else {
      if (!im_index->IsHyperLinked(0)) files.push_back(file);
      else
      {
        File::Ptr hyperlinked_file = File::Ptr(new File());
        if (!hyperlinked_file->OpenForReading(im_index->GetPathName(0))) return false;
        else files.push_back(hyperlinked_file);
      }

    	return true;
    }
  }

  bool DataBinServer::SetRequest(const Request& req)
  {
    bool res = true;
    bool reset_woi = false;

    data_writer.ClearPreviousIds();

    if((has_woi = req.mask.HasWOI())) {
      WOI new_woi;
      new_woi.size = req.woi_size;
      new_woi.position = req.woi_position;
      req.GetResolution(*im_index, &new_woi);

      if(new_woi != woi) {
        reset_woi = true;
        woi = new_woi;
      }
    }

    if(req.mask.items.model)
      cache_model += req.cache_model;

    if (req.mask.items.metareq)
      metareq = true;

    if(req.mask.items.stream || req.mask.items.context) {
      Range new_range = Range(req.min_codestream, req.max_codestream);

      if(new_range != range) {
        reset_woi = true;

        if(new_range.Length() != range.Length())
          files.resize(new_range.Length());

        range = new_range;
        current_idx = range.first;

        for(int i = 0; i < range.Length(); i++) {
          int idx = range.GetItem(i);

          if(!im_index->IsHyperLinked(idx)) files[i] = file;
          else {
            files[i] = File::Ptr(new File());
            res = res && files[i]->OpenForReading(im_index->GetPathName(idx));
          }
        }
      }
    }

    if(req.mask.items.len) {
      pending = req.length_response;
      has_len = true;
    }

    if(reset_woi) {
      end_woi_ = false;
      //woi_composer.Reset(woi, *im_index);
      /****/
      woi_composer = new WOIComposer[range.Length()];
      for(int i = 0; i < range.Length(); i++) {
    	  woi_composer[i].Reset(woi, *im_index);
    	  //cout << "[SetRequest] #: " << i << endl;
      }
      /****/
    }

    if (req.mask.items.mbw && req.mask.items.srate) {
    	uint64_t bytes;
    	switch(req.unit_bandwidth) {
    	  case 'T': bytes = (uint64_t)req.max_bandwidth << 40;
    		  	    break;
    	  case 'G': bytes = req.max_bandwidth << 30;
    		  	    break;
    	  case 'M': bytes = req.max_bandwidth << 20;
    		  	    break;
    	  case 'K': bytes = req.max_bandwidth << 10;
    		  	    break;
    	}
    	bytes = bytes >> 3;
    	bytes_per_frame = bytes / req.sampling_rate;

    	cout << "**** bytes_per_frame: " << bytes_per_frame;
    } else {
    	// TEST
    	bytes_per_frame = 8000;
    }

    return res;
  }
	
  bool DataBinServer::GenerateChunk(char *buff, int *len, bool *last)
  {
	/****/
	if (!has_len)
	  pending = REQUEST_LEN_SIZE;
    /****/

	data_writer.SetBuffer(buff, min(pending, *len));

    if(pending > 0) {
      eof = false;

      if(!im_index->ReadLock(range)) {
        ERROR("The lock of the image '" << im_index->GetPathName() << "' can not be taken for reading");
        return false;
      }

      if(im_index->GetNumMetadatas() <= 0)
        WriteSegment<DataBinClass::META_DATA>(0, 0, FileSegment::Null);
      else {
    	int bin_offset = 0;
        bool last_metadata = false;

        for (int i = 0; i < im_index->GetNumMetadatas(); i++)
        {
          last_metadata = (i == (im_index->GetNumMetadatas() - 1));
          WriteSegment<DataBinClass::META_DATA>(0, 0, im_index->GetMetadata(i), bin_offset, last_metadata);
          bin_offset += im_index->GetMetadata(i).length;

          if (!last_metadata)
          {
            if (WritePlaceHolder(0, 0, im_index->GetPlaceHolder(i), bin_offset) <= 0) break;					
            bin_offset += im_index->GetPlaceHolder(i).length();
          }
        }
				
        /*if (metareq)
        {
          for (int i = 0; i < im_index->GetNumMetadatas()-1; i++)
          {
        	  PlaceHolder place_holder=im_index->GetPlaceHolder(i);
        	  if (!place_holder.is_jp2c)
        	  {
        		  int res;
        		  res=WriteSegment<DataBinClass::META_DATA>(0, place_holder.id, FileSegment(place_holder.header.offset, place_holder.header.length + place_holder.data_length), 0, true);
        		  cout<<res<<" - "<<eof<<endl;
        		  cout<<place_holder.id<<" - "<<place_holder.header.offset<<" - "<<place_holder.header.length<<" - "<<place_holder.data_length<<endl;
        	  }
          }
        }*/
      }

      if (!eof)
      {
    	// TODO: Optimize
    	for (int i = range.first; i <= range.last; i++)
        {
          WriteSegment<DataBinClass::MAIN_HEADER>(i, 0, im_index->GetMainHeader(i));
          WriteSegment<DataBinClass::TILE_HEADER>(i, 0, FileSegment::Null);
        }

        if(has_woi) {
          int res;
          Packet packet;
          FileSegment segment;
          int bin_id, bin_offset;
          bool last_packet = false;

          // TODO: Calculate first packet to initialize "sum"

          while(data_writer && !eof) {
            packet= woi_composer[current_idx].GetCurrentPacket();

            /****/
            //cout << dec << "\n[GetCurrentPacket] #: " << current_idx << "\tR: " << packet.resolution << "\tX: " << packet.precinct_xy.x << "\tY: " << packet.precinct_xy.y;
            //cout << "\tC: " << packet.component << "\tL: " << packet.layer << endl;
            /****/

            segment = im_index->GetPacket(current_idx, packet, &bin_offset);

            bin_id = im_index->GetCodingParameters()->GetPrecinctDataBinId(packet);

            last_packet = (packet.layer >= (im_index->GetCodingParameters()->num_layers - 1));

            // DELETE THE LAST PARAMETER
            /****/
            res = WriteSegment<DataBinClass::PRECINCT>(current_idx, bin_id, segment, bin_offset, last_packet, true);
            /****/
            //res = WriteSegment<DataBinClass::PRECINCT>(current_idx, bin_id, segment, bin_offset, last_packet);

            /****/
            //cout << "data_writer.GetCount(): [" << data_writer.GetCount() << "]" << "\t res: " << res << "\t data_writer: " << data_writer << "\t eof: " << eof << endl;
            /****/

            cout << "**** bytes_per_frame: " << bytes_per_frame << endl;

            if(res < 0) return false;
            else if(res > 0) {

            	if (sum >= bytes_per_frame) {
                  if (current_idx != range.last) {
                    current_idx++;
                  } else {
                	current_idx = range.first;
                  }
                  /****/
          	      //cout << "[sum >= bytes_per_frame] " << sum << " >= " << bytes_per_frame << endl;
                  /****/
          	      sum = 0;
                  continue;
            	}

                if(!woi_composer[current_idx].GetNextPacket()) {
                  if (current_idx != range.last) {
                    current_idx++;
                    /****/
                    //cout << dec << "\t** " << current_idx << " **************************" << endl;
                    /****/
                  } else {
                	current_idx = range.first;
                	break;
                  }
                }
            }

            // ORIGINAL
        	/*
            if(res < 0) return false;
            else if(res > 0) {
              if (current_idx != range.last) current_idx++;
              else
              {
                if(!woi_composer.GetNextPacket()) break;
                else current_idx = range.first;
              }
            }
            */
          }
          /****/
          sum = sum + data_writer.GetCount();
          //cout << dec << " #### [" << sum << "/" << segment.length << "] ####" << endl;
          /****/
        }
      }

       if(!eof) {
        data_writer.WriteEOR(EOR::WINDOW_DONE);
        end_woi_ = true;
        pending = 0;
      } else {
      	pending -= data_writer.GetCount();

        if(pending <= MINIMUM_SPACE + 100) {
          data_writer.WriteEOR(EOR::BYTE_LIMIT_REACHED);
          pending = 0;
        }
      }

      if(!im_index->ReadUnlock(range)) {
        ERROR("The lock of the image '" << im_index->GetPathName() << "' can not be released");
        return false;
      }
    }

    *len = data_writer.GetCount();
    *last = (pending <= 0);

    if(*last) cache_model.Pack();

    /****/
    //cout << dec << "\t[return][" << data_writer.GetCount() << "]" << hex << " Hex: [" << data_writer.GetCount() << "]";
    //cout << "\tdata_writer: " << data_writer << "\teof: " << eof << endl;
    /****/

    return data_writer;
  }

}
