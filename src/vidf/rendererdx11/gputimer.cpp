#include "pch.h"
#include "gputimer.h"

namespace vidf { namespace dx11 {



GpuTimer::GpuTimer(RenderDevicePtr _renderDevice)
	: renderDevice(_renderDevice)
{
	D3D11_QUERY_DESC queryDesc{};
	queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	renderDevice->GetDevice()->CreateQuery(&queryDesc, &frequencyQuery.Get());
}

	

void GpuTimer::Begin()
{
	assert(!quering);
	renderDevice->GetContext()->Begin(frequencyQuery);
	quering = true;
}



void GpuTimer::End()
{
	assert(quering);
	renderDevice->GetContext()->End(frequencyQuery);
	quering = false;
		
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
	while (renderDevice->GetContext()->GetData(frequencyQuery, &data, sizeof(data), 0) != S_OK);
	frequency = data.Frequency;
	for (uint i = 0; i < timeStamps.size(); ++i)
		while (renderDevice->GetContext()->GetData(queries[i], &timeStamps[i], sizeof(uint64), 0) != S_OK);
}



GpuTimer::TimeStampId GpuTimer::MakeNewTimeStamp()
{
	timeStamps.push_back(0);

	D3D11_QUERY_DESC queryDesc{};
	queryDesc.Query = D3D11_QUERY_TIMESTAMP;
	ID3D11Query* query;
	renderDevice->GetDevice()->CreateQuery(&queryDesc, &query);
	queries.push_back(query);

	return timeStamps.size() - 1;
}



void GpuTimer::PushTimeStamp(TimeStampId timeStampId)
{
	assert(quering);
	renderDevice->GetContext()->End(queries[timeStampId]);
}



GpuSection::GpuSection(GpuTimer* _timer)
	:	timer(_timer)
	,	beginId(_timer->MakeNewTimeStamp())
	,	endId(_timer->MakeNewTimeStamp())
{
}



float GpuSection::AsFloat() const
{
	const uint64 begin = timer->GetTimeStamp(beginId);
	const uint64 end = timer->GetTimeStamp(endId);
	const uint64 freq = timer->GetFrequency();
	return float(double(end - begin) / double(freq));
}



} }
